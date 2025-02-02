#include <stdint.h>
#include <stddef.h>

#include "utils.h"
#include "platform.h"

#define ATTR_DEVICE           0x0
#define ATTR_NORMAL_CACHEABLE 0xff

#define INDEX_ATTR_DEVICE           0
#define INDEX_ATTR_NORMAL_CACHEABLE 1

static __attribute__((aligned(4096))) uint64_t mmuspace[0x80000];

enum shareability_e: uint64_t
{
    SH_NON_SHAREABLE = 0,
    SH_RESERVED = 1,
    SH_OUTER_SHAREABLE = 2,
    SH_INNER_SHAREABLE = 3
};

enum descriptor_type_e: uint64_t
{
    DESC_TYPE_BLOCK = 1,
    DESC_TYPE_PAGE = 3,
    DESC_TYPE_TABLE = 3
};

// Called only when giving up (not enough MMU space, etc.). The MMU
// configuration code is assumed to be run from EL2 so it will run an SMC
// instruction to notify the secure monitor (EL3). Although that part would
// certainly require a stub on the SD card to be handled properly.
static inline void smc0(){ __asm__("smc #0"); }

static inline uint64_t level_offset(uint64_t address, uint64_t level)
{
    return (address >> (39 - 9 * level)) & 0x1ff;
}

static uint64_t *alloc_descriptors(size_t n)
{
    static uint64_t *cursor = &mmuspace[1 << 9];
    const uint64_t *end = mmuspace + sizeof(mmuspace) / sizeof(mmuspace[0]);

    // Assumes no address overflow
    if(cursor + n > end) // Not enough MMU space
        smc0();
    uint64_t *tmp = cursor;
    cursor += n;
    return tmp;
}

// Goes through MMU levels from level 0 to reach for "address".
// Returns a pointer to the first block/page entry.
// Modifies leaf_level to indicate at which level (1, 2 or 3)
// the block/page entry has been found.
static uint64_t *get_leaf_for(
    uint64_t address,
    size_t *leaf_level
)
{
    uint64_t *ptr = &mmuspace[0];
    for(size_t i = 0; i < 4; ++i)
    {
        uint64_t *pdescriptor = &ptr[level_offset(address, i)];
        uint64_t descriptor = *pdescriptor;
        enum descriptor_type_e type = (descriptor & 0x3);
        // Sanity check
        if(i == 0 && type != DESC_TYPE_TABLE)
            smc0();
        if(i != 3 && type == DESC_TYPE_TABLE)
            ptr = (uint64_t*)(descriptor & 0xfffffffff000ul);
        else
        {
            *leaf_level = i;
            return pdescriptor;
        }
    }
    // Should not be run
    smc0();
    *leaf_level = 4;
    return NULL;
}

static inline uint64_t descriptor_block_attributes(
    uint64_t memory_attr_index,
    uint64_t shareability,
    bool execute
)
{
    return ((memory_attr_index & 0x7) << 2)
         | ((shareability & 0x3) << 8) // SH
         | (1 << 10) // Pre-set AF=1. FEAT_HAFDBS will be supported in ARMv8.1
         | ((execute ? 0ul : 1ul) << 54) // XN
         | (((memory_attr_index >> 3) & 1) << 59)
         ;
}

static inline void register_setup(
    uint64_t mair,
    uint64_t tcr,
    uint64_t hcr_clear,
    uint64_t hcr_set
)
{
    uint64_t hcr = 0;

    __asm__("msr MAIR_EL1, %x0" :: "r"(mair));
    __asm__("msr TTBR0_EL1, %x0" :: "r"(mmuspace));
    __asm__("msr TCR_EL1, %x0" :: "r"(tcr));

    isb();
    __asm__("mrs %x0, HCR_EL2" : "=r"(hcr));
    hcr &= ~hcr_clear;
    hcr |= hcr_set;
    __asm__("msr HCR_EL2, %x0" :: "r"(hcr));
}

static void fill_with_identity_blocks(
    uint64_t base_address,
    uint64_t index_shift,
    size_t ndescriptors
)
{
    uint64_t *ptr = alloc_descriptors(ndescriptors);
    for(uint64_t i = 0; i < ndescriptors; ++i)
    {
        uint64_t input_address = base_address | (i << index_shift);
        *ptr++ = DESC_TYPE_BLOCK
               | descriptor_block_attributes(
                     INDEX_ATTR_NORMAL_CACHEABLE,
                     SH_OUTER_SHAREABLE,
                     true)
               | input_address
               ;
    }
}

static inline uint64_t level_to_size(uint64_t level)
{
    return (1ul << (39 - 9 * level));
}

static inline uint64_t level_to_clear_mask(uint64_t level)
{
    return 0xfffffffff000ul & ~(level_to_size(level) - 1ul);
}

static inline size_t alignment_to_level(uint64_t address)
{
    // CLZ(0) returns 64. So CLZ(RBIT(0)) also returns 64, which would return 1
    // as expected.

    uint64_t first_bit_set = 47;
    __asm__("rbit x1, %x1\n"
            "clz %x0, x1"
            : "=r"(first_bit_set)
            : "r"(address & 0xfffffffff000ul)
            : "x1"
    );
    // Level0 doesn't count because level0 descriptors are tables
    if(first_bit_set >= 30)
        return 1;
    if(first_bit_set >= 21)
        return 2;
    return 3;
}

static void add_pages(uint64_t address_start, uint64_t address_end, uint64_t attributes)
{
    address_start &= level_to_clear_mask(3);
    address_end = (address_end + level_to_size(3) - 1) & level_to_clear_mask(3);
    uint64_t size = address_end - address_start;

    while(size)
    {
        // Pre-compute level based on address alignment
        size_t level_to_allocate = alignment_to_level(address_start);

        // Adjust level based on size requirements
        while(level_to_allocate < 3 && level_to_size(level_to_allocate) > size)
            ++level_to_allocate;

        // Force address alignment
        address_start &= level_to_clear_mask(level_to_allocate);

        size_t leaf_level;
        uint64_t *leaf_descriptor = get_leaf_for(
            address_start,
            &leaf_level
        );
        for(size_t lvl = leaf_level; lvl < level_to_allocate; ++lvl)
        {
            // Sanity check: must be a block descriptor at that point
            enum descriptor_type_e descriptor_type = (*leaf_descriptor & 0x3);
            if(descriptor_type != DESC_TYPE_BLOCK)
                smc0();

            // Allocate and initialize the new table with the same block
            // attributes
            uint64_t block_attributes = (*leaf_descriptor & 0xfffc000000000ffcul);
            uint64_t block_output_address =
                (*leaf_descriptor & level_to_clear_mask(lvl));
            uint64_t *new_table = alloc_descriptors(1 << 9);
            for(uint64_t i = 0; i < (1 << 9); ++i)
            {
                new_table[i] = (lvl + 1 == 3 ? DESC_TYPE_PAGE : DESC_TYPE_BLOCK)
                             | block_attributes
                             | block_output_address
                             | (i << (39 - 9 * (lvl + 1)))
                             ;
            }

            // Replace the old leaf descriptor with a pointer to the new table
            *leaf_descriptor = DESC_TYPE_TABLE | (uint64_t) new_table;

            // Update the leaf descriptor pointer to next level entry
            leaf_descriptor = &new_table[level_offset(address_start, lvl + 1)];
        }
        *leaf_descriptor =
            (level_to_allocate == 3 ? DESC_TYPE_PAGE : DESC_TYPE_BLOCK)
            | address_start
            | attributes
            ;

        uint64_t local_size = level_to_size(level_to_allocate);
        address_start += local_size;
        size -= local_size;
    }
}

// TODO User and Privileged table partitioning (TTBR0 and TTBR1)
void mmu_init_registers()
{
    uint64_t mair = (ATTR_DEVICE << (INDEX_ATTR_DEVICE << 3))
                  | (ATTR_NORMAL_CACHEABLE << (INDEX_ATTR_NORMAL_CACHEABLE << 3))
                  ;

    // For now, just map the entire 48b memory space
    uint64_t tcr = 16 // T0SZ
                 | (1ul << 8) // IRGN0
                 | (1ul << 10) // ORGN0
                 | (2ul << 12) // SH0
                 | (0ul << 14) // TG0
                 | (5ul << 32) // IPS
                 | (0ul << 37) // TBI0 (ignore top byte for prefixed addresses like
                             // 0xfe...)
                 ;

    uint64_t hcr_clear = (1ul << 38) // MIOCNCE
                       | (1ul << 0) // VM
                       ;
    uint64_t hcr_set = (1ul << 30) // TRVM
                     | (1ul << 26) // TVM
                     ;
    register_setup(mair, tcr, hcr_clear, hcr_set);
}

void mmu_init_descriptors()
{
    // Level0 descriptor can only be table descriptors
    uint64_t *mmuptr = &mmuspace[0];
    for(uint64_t lvl0_i = 0; lvl0_i < (1 << 9); ++lvl0_i)
    {
        uint64_t next_level_table = (uint64_t) &mmuspace[((lvl0_i + 1) << 9)];
        *mmuptr++ = DESC_TYPE_TABLE
                  | next_level_table
                  ;
    }

    // Level1 descriptors will be mostly block descriptors in order to make
    // things easier and the MMU table smaller.
    fill_with_identity_blocks(
        0x0,
        30,
        1 << 18
    );

    // Add exclusive section
    // ARM ARM E2.10.2 Exclusive access instructions and shareable memory
    // locations
    //
    // the only memory types for which it is architecturally guaranteed that
    // a global Exclusives monitor is implemented are:
    //  - Inner Shareable, Inner Write-Back, Outer Write-Back Normal memory
    //    with Read allocation hint and Write allocation hint and not transient.
    //  - Outer Shareable, Inner Write-Back, Outer Write-Back Normal memory with
    //    Read allocation hint and Write allocation hints and not transient.

    extern void *__exclusive_start; // From LD script
    extern void *__exclusive_end; // From LD script
    uint64_t exclusive_start_address = (uint64_t) &__exclusive_start;
    uint64_t exclusive_end_address = (uint64_t) &__exclusive_end;

    add_pages(
        exclusive_start_address,
        exclusive_end_address,
        descriptor_block_attributes(
            INDEX_ATTR_NORMAL_CACHEABLE,
            SH_OUTER_SHAREABLE,
            false
        )
    );

    // Add peripheral/device section
    add_pages(
        PERIPHERAL_BASE,
#if RPI < 5
        PERIPHERAL_BASE + 0x1000000,
#else
        PERIPHERAL_BASE + 0x4000000,
#endif
        descriptor_block_attributes(
            INDEX_ATTR_DEVICE,
            SH_NON_SHAREABLE,
            false
        )
    );
#if RPI >= 5
    add_pages(
        RP1_BASE,
        RP1_BASE + 0x200000,
        descriptor_block_attributes(
            INDEX_ATTR_DEVICE,
            SH_NON_SHAREABLE,
            false
        )
    );
#endif
    add_pages(
        0x3e000000,
        0x3f000000,
        descriptor_block_attributes(
            INDEX_ATTR_DEVICE,
            SH_NON_SHAREABLE,
            false
        )
    );

    extern void *__device_start; // From LD script
    extern void *__device_end; // From LD script
    uint64_t device_start_address = (uint64_t) &__device_start;
    uint64_t device_end_address = (uint64_t) &__device_end;
    add_pages(
        device_start_address,
        device_end_address,
        descriptor_block_attributes(
            INDEX_ATTR_DEVICE,
            SH_NON_SHAREABLE,
            false
        )
    );

    memoryBarrier();
}

