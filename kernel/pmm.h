#ifndef _PMM_H_
#define _PMM_H_
#include "memlayout.h"
#include "riscv.h"
// Initialize phisical memeory manager
void pmm_init();
// Allocate a free phisical page
void* alloc_page();
// Free an allocated page
void free_page(void* pa);
// NOTE reference counter for pages
struct mem_ref_t
{
  // NOTE if we want to implement COW in multicore, we should add a spinlock in struct
  int cnt;
};
extern struct mem_ref_t mem_ref[];
#endif