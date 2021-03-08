#include <drivers/common.h>

#include "vpu.h"
#include "mailbox.h"

#define MEM_FLAG_DISCARDABLE      (1 << 0) /* can be resized to 0 at any time. Use for cached data */
#define MEM_FLAG_NORMAL           (0 << 2) /* normal allocating alias. Don't use from ARM */
#define MEM_FLAG_DIRECT           (1 << 2) /* 0xC alias uncached */
#define MEM_FLAG_COHERENT         (2 << 2) /* 0x8 alias. Non-allocating in L2 but coherent */
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT), /* Allocating in L2 */
#define MEM_FLAG_ZERO             (1 << 4) /* initialize buffer to all zeros */
#define MEM_FLAG_NO_INIT          (1 << 5) /* don't initialise (default is initialise to all ones) */
#define MEM_FLAG_HINT_PERMALOCK   (1 << 6) /* Likely to be locked for long periods of time. */

int vpu_execute_code(const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t *ret)
{
    volatile uint32_t __attribute__((aligned(16))) sequence[] = {
        0, // size of the whole structure
        0, // request
        
        // TAG 0
        0x00030010, // Execute code
        28, // size
        0, // request
        (uint32_t) function_pointer,
        r0,
        r1,
        r2,
        r3,
        r4,
        r5,

        0 // End TAG
    };
    sequence[0] = sizeof(sequence);
    

    // Send the requested values
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence));
    if(mailbox_receive(8) == 0 || sequence[1] == 0x80000000) // Ok
    {
        volatile const uint32_t *ptr = sequence + 2;
        while(*ptr)
        {
            switch(*ptr++)
            {
                case 0x00030010:
                    *ret = ptr[2];
                    break;

                default: // Panic
                    return 1;
            }
            ptr += *ptr / 4 + 2;
        }
        return 0;
    }
    return 1;
}

int vpu_execute_code_with_stack(size_t stack_size, const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t *ret)
{
    volatile const uint32_t *ptr;
    uint32_t stack_handle = -1u;
    uint32_t stack_address = -1u;

    stack_size = (stack_size + 3) & ~0x3ul;

    // 1. Allocate stack
    volatile uint32_t __attribute__((aligned(16))) sequence0[] = {
        0, // size of the whole structure
        0, // request
        
        0x0003000c, // Allocate memory
        12, // size
        0, // request
        stack_size,
        4, // alignment
        MEM_FLAG_NORMAL | MEM_FLAG_NO_INIT,

        0 // End TAG
    };
    sequence0[0] = sizeof(sequence0);
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence0));
    if(mailbox_receive(8) != 0 && sequence0[1] != 0x80000000)
        return 1;
    ptr = sequence0 + 2;
    while(*ptr)
    {
        switch(*ptr++)
        {
            case 0x0003000c:
                stack_handle = ptr[2];
                break;

            default: // Panic
                return 2;
        }
        ptr += *ptr / 4 + 2;
    }

    // 2. Get stack address (lock)
    volatile uint32_t __attribute__((aligned(16))) sequence1[] = {
        0, // size of the whole structure
        0, // request
        
        0x0003000d, // Lock memory
        4, // size
        0, // request
        stack_handle,

        0 // End TAG
    };
    sequence1[0] = sizeof(sequence1);
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence1));
    if(mailbox_receive(8) != 0 && sequence1[1] != 0x80000000)
        return 3;
    ptr = sequence1 + 2;
    while(*ptr)
    {
        switch(*ptr++)
        {
            case 0x0003000d:
                stack_address = ptr[2];
                break;

            default: // Panic
                return 4;
        }
        ptr += *ptr / 4 + 2;
    }

    // 3. Run
    if(vpu_execute_code(function_pointer, r0, r1, r2, r3, r4, stack_address + stack_size, ret))
        return 5;

    // 4. Unlock stack
    volatile uint32_t __attribute__((aligned(16))) sequence2[] = {
        0, // size of the whole structure
        0, // request
        
        0x0003000e, // Unlock memory
        4, // size
        0, // request
        stack_handle,

        0 // End TAG
    };
    sequence2[0] = sizeof(sequence2);
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence2));
    if(mailbox_receive(8) != 0 && sequence2[1] != 0x80000000)
        return 6;
    ptr = sequence2 + 2;
    while(*ptr)
    {
        switch(*ptr++)
        {
            case 0x0003000e:
                break;

            default: // Panic
                return 7;
        }
        ptr += *ptr / 4 + 2;
    }

    // 5. Free stack
    volatile uint32_t __attribute__((aligned(16))) sequence3[] = {
        0, // size of the whole structure
        0, // request
        
        0x0003000f, // Free memory
        4, // size
        0, // request
        stack_handle,

        0 // End TAG
    };
    sequence3[0] = sizeof(sequence3);
    mailbox_send(8, VIDEOBUS_OFFSET + ((uint32_t)sequence3));
    if(mailbox_receive(8) != 0 && sequence3[1] != 0x80000000)
        return 8;
    ptr = sequence3 + 2;
    while(*ptr)
    {
        switch(*ptr++)
        {
            case 0x0003000f:
                break;

            default: // Panic
                return 9;
        }
        ptr += *ptr / 4 + 2;
    }

    return 0;
}

