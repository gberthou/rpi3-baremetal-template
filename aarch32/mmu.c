#include <stdint.h>
#include <stddef.h>

#include "utils.h"
#include "platform.h"

/* TTBCR.N=2, compatibility with 4kb granules
 * /!\ When N!=0, TTBR1 is used for higher addresses
 * (N=2 => Addresses over 0x40000000 use TTBR1, which implies that 75% of
 * addresses are covered by TTBR1 and 25% are covered by TTBR0)
 */
#define TTBCR_N 2
#define N_MMU_ENTRIES 0x80000
/* Compute the amount of TTBR0 entries as approximately the adequate proportion
 * of total entries, while aligning the TTBR1 address to 14 bits (N=0)
 */
#define N_MMU_ENTRIES_TTBR0 (((N_MMU_ENTRIES >> TTBCR_N) + 0x3fff) & ~0x3fff)
#define N_LEVEL0_TTBR0 (1 << (12 - TTBCR_N))
#define N_LEVEL0_TTBR1 (1 << 12) // N = 0

static __attribute__((aligned(1 << 14))) uint32_t mmuspace[N_MMU_ENTRIES];

enum descriptor_type_e: uint32_t
{
    DESC_TYPE_BLOCK = 1,
    DESC_TYPE_COARSE_PAGE_TABLE = 1, // Armv6
    DESC_TYPE_SECTION = 2, // Armv6
    DESC_TYPE_SMALL_PAGE = 2, // Armv6
    DESC_TYPE_PAGE = 3,
    DESC_TYPE_TABLE = 3
};

// Called only when giving up (not enough MMU space, etc.). The MMU
// configuration code is assumed to be run from EL2 so it will run an SMC
// instruction to notify the secure monitor (EL3). Although that part would
// certainly require a stub on the SD card to be handled properly.
static inline void smc0(){ __asm__("smc #0"); }

static uint32_t *alloc_descriptors(bool ttbr0, size_t n)
{
    static uint32_t *cursor0 = &mmuspace[N_LEVEL0_TTBR0];
    static uint32_t *cursor1 = &mmuspace[N_MMU_ENTRIES_TTBR0 + N_LEVEL0_TTBR1];
    const uint32_t *end0 = mmuspace + N_MMU_ENTRIES_TTBR0;
    const uint32_t *end1 = mmuspace + N_MMU_ENTRIES;

    uint32_t **cursor = ttbr0 ? &cursor0 : &cursor1;
    const uint32_t *end = ttbr0 ? end0 : end1;

    // Assumes no address overflow
    if(*cursor + n > end) // Not enough MMU space
        smc0();
    uint32_t *tmp = *cursor;
    *cursor += n;
    return tmp;
}

static inline uint32_t descriptor_section(
    uint32_t b,
    uint32_t c,
    bool execute,
    uint32_t domain,
    uint32_t ap_x,
    uint32_t tex,
    uint32_t shareability
)
{
     return ((b & 1) << 2)
          | ((c & 1) << 3)
          | ((execute ? 0 : 1) << 4) // XN
          | ((domain & 0xf) << 5)
          | ((ap_x & 0x3) << 10)
          | ((tex & 0x7) << 12)
          | (((ap_x >> 2) & 1) << 15)
          | ((shareability & 1) << 16)
          ;
}

static inline uint32_t descriptor_section_to_coarse(uint32_t descriptor)
{
    return descriptor & 0x3e0;
}

static inline uint32_t descriptor_section_to_small(uint32_t descriptor)
{
    uint32_t xn = (descriptor >> 4) & 1;
    uint32_t ap = (descriptor >> 10) & 0x3;
    uint32_t tex = (descriptor >> 12) & 0x7;
    uint32_t apx = (descriptor >> 15) & 1;
    uint32_t s = (descriptor >> 16) & 1;
    return xn
         | 2
         | (descriptor & 0xc)
         | (ap << 4)
         | (tex << 6)
         | (apx << 9)
         | (s << 10)
         ;
}

static inline bool can_edit_hyp_registers()
{
    uint32_t cpsr;
    __asm__("mrs %0, cpsr" : "=r"(cpsr));
    uint32_t mode = cpsr & 0xf;
    return mode == 0xa || mode == 0x6;
}

static inline void register_setup(
    uint32_t dacr,
    uint32_t tcr,
    uint64_t hcr_clear,
    uint64_t hcr_set
)
{
    uint32_t hcr = 0;
    uint32_t hcr2 = 0;
    uint32_t actlr = 0;
    uint32_t ttbr0 = ((uint32_t) mmuspace)
                  | (0x01 << 0) // IRGN=0b01 (Multiprocessing Extensions Enabled format)
                  | (1ul << 3) // RGN
                  | (1ul << 1) // S
                  ;
    uint32_t ttbr1 = ((uint32_t) &mmuspace[N_MMU_ENTRIES_TTBR0])
                  | (0x01 << 0) // IRGN=0b01 (Multiprocessing Extensions Enabled format)
                  | (1ul << 3) // RGN
                  | (1ul << 1) // S
                  ;

    __asm__("mcr p15, 0, %0, c3, c0, 0" :: "r"(dacr));
    __asm__("mcr p15, 0, %0, c2, c0, 0" :: "r"(ttbr0));
    __asm__("mcr p15, 0, %0, c2, c0, 1" :: "r"(ttbr1));
    __asm__("mcr p15, 0, %0, c2, c0, 2" :: "r"(tcr));

    isb();
    __asm__("mrc p15, 0, %0, c1, c0, 1" : "=r"(actlr));
    actlr |= (1 << 5); // SMP/nAMP mode
    __asm__("mcr p15, 0, %0, c1, c0, 1" :: "r"(actlr));

    if(can_edit_hyp_registers())
    {
        __asm__("mrc 15, 4, %0, c1, c1, 0\n"
                "mrc 15, 4, %1, c1, c1, 4" : "=r"(hcr), "=r"(hcr2));
        hcr &= ~hcr_clear;
        hcr |= hcr_set;
        hcr2 &= ~(hcr_clear >> 32);
        hcr2 |= hcr_set >> 32;
        __asm__("mcr 15, 4, %0, c1, c1, 0\n"
                "mcr 15, 4, %1, c1, c1, 4" :: "r"(hcr), "r"(hcr2));
    }
}

static inline uint32_t level_to_size(bool ttbr0, uint32_t level)
{
    if(level == 0)
        return 1 << 20;
    // For TTBR1-managed addresses, calculations consider TTBCR.N=0
    return 1 << (14 - (ttbr0 ? TTBCR_N : 0));
}

static inline uint32_t level_to_clear_mask(bool ttbr0, uint32_t level)
{
    return (ttbr0 ? ~0xfff : ~0x3fff) & ~(level_to_size(ttbr0, level) - 1);
}

static inline uint32_t min_address_for_ttbr1()
{
    if(TTBCR_N == 0)
        return 0xffffffff;
    return 1u << (32 - TTBCR_N);
}

static inline uint32_t address_mask_for_ttbr0()
{
    if(TTBCR_N == 0)
        return 0xffffffff;
    return min_address_for_ttbr1() - 1;
}

static inline size_t alignment_to_level(uint32_t address)
{
    if(address < min_address_for_ttbr1())
        address &= ~0x3fff;
    if(!(address & 0xff000))
        return 0;
    return 1;
}

static inline uint32_t *get_lvl0_descriptor(uint32_t address)
{
    if(address < min_address_for_ttbr1())
        return &mmuspace[(address & address_mask_for_ttbr0()) >> 20];
    // For TTBR1-managed addresses, calculations consider TTBCR.N=0
    return &mmuspace[N_MMU_ENTRIES_TTBR0 + (address >> 20)];
}

static void add_pages(uint64_t address_start, uint64_t address_end, uint32_t attributes)
{
    bool ttbr0 = (address_start < min_address_for_ttbr1());
    address_start &= level_to_clear_mask(ttbr0, 1);
    address_end = (address_end + level_to_size(ttbr0, 1) - 1)
                & level_to_clear_mask(ttbr0, 1);
    uint32_t size = address_end - address_start;

    while(size)
    {
        // Pre-compute level based on address alignment
        size_t level_to_allocate = alignment_to_level(address_start);
        ttbr0 = (address_start < min_address_for_ttbr1());

        // Adjust level based on size requirements
        while(level_to_allocate < 1 && level_to_size(ttbr0, level_to_allocate) > size)
            ++level_to_allocate;

        // Force address alignment
        address_start &= level_to_clear_mask(ttbr0, level_to_allocate);

        uint32_t *lvl0_descriptor = get_lvl0_descriptor(address_start);
        if(level_to_allocate == 0)
        {
            *lvl0_descriptor = DESC_TYPE_SECTION
                             | attributes
                             | address_start
                             ;
        }
        else // 1
        {
            uint32_t lvl0_type = (*lvl0_descriptor & 0x3);
            uint32_t *table;
            if(lvl0_type == DESC_TYPE_COARSE_PAGE_TABLE) // Table already exists
                table = (uint32_t*)(*lvl0_descriptor & 0xfffffc00);
            else
            {
                // There are only 256 lvl1 table indices; however the coarse
                // page table base address format requires the addresses to be
                // aligned at 12 bits.
                table = alloc_descriptors(ttbr0, 1 << 12);
                for(uint32_t i = 0; i < (1 << 8); ++i)
                {
                    uint32_t address = (address_start & level_to_clear_mask(ttbr0, 0))
                                     | (i << 12)
                                     ;
                    table[i] = DESC_TYPE_SMALL_PAGE
                             | descriptor_section_to_small(*lvl0_descriptor)
                             | address
                             ;
                }

                *lvl0_descriptor = DESC_TYPE_COARSE_PAGE_TABLE
                                 | descriptor_section_to_coarse(*lvl0_descriptor)
                                 | (uint32_t) table
                                 ;
            }

            uint32_t *lvl1_descriptor = &table[(address_start >> 12) & 0xff];
            *lvl1_descriptor = DESC_TYPE_SMALL_PAGE
                             | descriptor_section_to_small(attributes)
                             | (address_start & 0xfffff000)
                             ;
        }

        uint32_t local_size = level_to_size(ttbr0, level_to_allocate);
        address_start += local_size;
        size -= local_size;
    }
}

// TODO User and Privileged table partitioning (TTBR0 and TTBR1)
void mmu_init_registers()
{
    uint32_t dacr = 0x1; // Domain0 as client
    uint32_t tcr = TTBCR_N;
    uint64_t hcr_clear = (1ull << 38) // MIOCNCE
                       | (1ul << 27) // TGE
                       | (1ul << 12) // DC
                       | (1ul << 0) // VM
                       ;
    uint64_t hcr_set = 0;
    register_setup(dacr, tcr, hcr_clear, hcr_set);
}

void mmu_init_descriptors()
{
    for(size_t ttbr = 0; ttbr < (TTBCR_N == 0 ? 1 : 2); ++ttbr)
    {
        // Initialize TTBRn level0 with section (1MB) identity descriptors
        uint32_t *mmuptr = &mmuspace[ttbr == 0 ? 0 : N_MMU_ENTRIES_TTBR0];
        for(uint32_t lvl0_i = 0;
            lvl0_i < (ttbr == 0 ? N_LEVEL0_TTBR0 : N_LEVEL0_TTBR1);
            ++lvl0_i)
        {
            uint32_t address = (lvl0_i << 20);
            *mmuptr++ = DESC_TYPE_SECTION
                      | descriptor_section(1, 1, true, 0, 3, 1, 1)
                      | address
                      ;
        }
    }

    // Add exclusive section
    // DDI0360F 8.3 Synchronization operations
    //
    // These synchronization primitives can be used in coherent or noncachable
    // regions. Coherent regions are defined as Cachable, Write-Back
    // Write-Allocate shared memory regions, when the SMP/nAMP bit [5] is set.

    extern void *__exclusive_start; // From LD script
    extern void *__exclusive_end; // From LD script
    uint32_t exclusive_start_address = (uint32_t) &__exclusive_start;
    uint32_t exclusive_end_address = (uint32_t) &__exclusive_end;

    add_pages(
        exclusive_start_address,
        exclusive_end_address,
        descriptor_section(1, 1, false, 0, 3, 1, 1)
    );

    // Add peripheral/device section
    add_pages(
        PERIPHERAL_BASE,
        PERIPHERAL_BASE + 0x1000000,
        descriptor_section(0, 0, false, 0, 3, 0, 0)
    );
    add_pages(
        0x3e000000,
        0x3f000000,
        descriptor_section(0, 0, false, 0, 3, 0, 0)
    );

    extern void *__device_start; // From LD script
    extern void *__device_end; // From LD script
    uint32_t device_start_address = (uint32_t) &__device_start;
    uint32_t device_end_address = (uint32_t) &__device_end;
    add_pages(
        device_start_address,
        device_end_address,
        descriptor_section(0, 0, false, 0, 3, 0, 0)
    );

    memoryBarrier();
}

