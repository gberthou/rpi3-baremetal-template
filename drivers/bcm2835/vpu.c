#include <drivers/common.h>

#include "vpu.h"
#include "mailbox.h"
#include "pointer.h"

#define MEM_FLAG_DISCARDABLE      (1 << 0) /* can be resized to 0 at any time. Use for cached data */
#define MEM_FLAG_NORMAL           (0 << 2) /* normal allocating alias. Don't use from ARM */
#define MEM_FLAG_DIRECT           (1 << 2) /* 0xC alias uncached */
#define MEM_FLAG_COHERENT         (2 << 2) /* 0x8 alias. Non-allocating in L2 but coherent */
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT), /* Allocating in L2 */
#define MEM_FLAG_ZERO             (1 << 4) /* initialize buffer to all zeros */
#define MEM_FLAG_NO_INIT          (1 << 5) /* don't initialise (default is initialise to all ones) */
#define MEM_FLAG_HINT_PERMALOCK   (1 << 6) /* Likely to be locked for long periods of time. */

#define MEM_HANDLE_INVALID 0xffffffff

const struct vpu_mem_t VPU_MEM_EMPTY = {0, NULL};

static bool vpu_malloc_allocate_cb(const uint32_t *message, void *context)
{
    if(*message == 0x0003000c)
    {
        uint32_t *stack_handle = context;
        *stack_handle = message[3];
        return true;
    }
    return false;
}

static bool vpu_malloc_lock_cb(const uint32_t *message, void *context)
{
    if(*message == 0x0003000d)
    {
        uint32_t *stack_address = context;
        *stack_address = message[3];
        return true;
    }
    return false;
}

struct vpu_mem_t vpu_malloc(size_t size)
{
    struct vpu_mem_t mem;
    uint32_t address = -1u;

    size = (size + 3) & ~0x3ul;

    // 1. Allocate
    const uint32_t request_allocate[] = {
        0, // Size of the whole structure (ignore)
        0, // Request
        
        0x0003000c, // Allocate memory
        12, // size
        0, // request
        size,
        4, // alignment
        MEM_FLAG_NORMAL | MEM_FLAG_NO_INIT,

        0 // End TAG
    };
    if(!mailbox_request(request_allocate, sizeof(request_allocate), vpu_malloc_allocate_cb, &mem.handle))
        return VPU_MEM_EMPTY;

    // 2. Get address (lock)
    const uint32_t request_lock[] = {
        0, // size of the whole structure
        0, // request
        
        0x0003000d, // Lock memory
        4, // size
        0, // request
        mem.handle,

        0 // End TAG
    };
    if(!mailbox_request(request_lock, sizeof(request_lock), vpu_malloc_lock_cb, &address))
    {
        mem.handle = MEM_HANDLE_INVALID;
        vpu_free(&mem);
        return VPU_MEM_EMPTY;
    }
    mem.ptr = u32_to_ptr(address);
    return mem;
}

bool vpu_free(const struct vpu_mem_t *mem)
{
    // 1. Unlock
    if(mem->handle != MEM_HANDLE_INVALID)
    {
        const uint32_t request_unlock[] = {
            0, // Size of the whole structure (ignore)
            0, // Request
            
            0x0003000e, // Unlock memory
            4, // size
            0, // request
            mem->handle,

            0 // End TAG
        };
        mailbox_request(request_unlock, sizeof(request_unlock), mailbox_ack, NULL);
    }

    // 2. Free
    if(mem->ptr != NULL)
    {
        const uint32_t request_free[] = {
            0, // Size of the whole structure (ignore)
            0, // Request
            
            0x0003000f, // Free memory
            4, // size
            0, // request
            mem->handle,

            0 // End TAG
        };
        return mailbox_request(request_free, sizeof(request_free), mailbox_ack, NULL);
    }
    return true;
}

static bool vpu_execute_code_cb(const uint32_t *message, void *context)
{
    if(*message == 0x00030010)
    {
        uint32_t *returnValue = context;
        *returnValue = message[3];
        return true;
    }
    return false;
}

int vpu_execute_code(const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t *ret)
{
    const uint32_t request[] = {
        0, // Size of the whole structure (ignore)
        0, // Request
        
        // TAG 0
        0x00030010, // Execute code
        28, // size
        0, // request
        ptr_to_u32(function_pointer),
        r0,
        r1,
        r2,
        r3,
        r4,
        r5,

        0 // End TAG
    };

    if(!mailbox_request(request, sizeof(request), vpu_execute_code_cb, ret))
        return 1;
    return 0;
}

int vpu_execute_code_with_stack(size_t stack_size, const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t *ret)
{
    // 1. Allocate stack
    struct vpu_mem_t mem_stack = vpu_malloc(stack_size);
    if(mem_stack.ptr == NULL)
        return 1;

    // 2. Run
    if(vpu_execute_code(function_pointer, r0, r1, r2, r3, r4, ptr_to_u32(mem_stack.ptr) + stack_size, ret))
        return 2;

    // 3. Free stack
    if(!vpu_free(&mem_stack))
        return 3;
    return 0;
}

