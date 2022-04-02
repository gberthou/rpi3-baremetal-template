#ifndef DRIVERS_VPU_H
#define DRIVERS_VPU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct vpu_mem_t
{
    uint32_t handle;
    void *ptr;
};

struct vpu_mem_t vpu_malloc(size_t size);
bool vpu_free(const struct vpu_mem_t *mem);

int vpu_execute_code(const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5, uint32_t *ret);
int vpu_execute_code_with_stack(size_t stack_size, const void *function_pointer, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t *ret);

#endif

