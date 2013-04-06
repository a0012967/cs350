#ifndef _PT_H_
#define _PT_H_

#include "types.h"

#define ALIGN(x)         ((x) & PAGE_FRAME)
#define IS_VALID(x)      ((x) & 0x00000001)
#define IS_DIRTY(x)      ((x) & 0x00000002)
#define IS_SWAPPED(x)    ((x) & 0x00000004)
#define SET_VALID(x)     ((x) | 0x00000001)
#define SET_DIRTY(x)     ((x) | 0x00000002)
#define SET_SWAPPED(x)   ((x) | 0x00000004)
#define SET_INVALID(x)   ((x) & 0xfffffffe)

struct pagetable;

struct pt_entry {
    vaddr_t vaddr;
    paddr_t paddr;
};

struct pagetable* pt_create();
void pt_destroy(struct pagetable* pt);
paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr, int *err);

#endif // _PT_H_
