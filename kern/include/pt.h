#ifndef _PT_H_
#define _PT_H_

#include "types.h"

#define IS_VALID(x) ((x) & 0x00000001)
#define SET_VALID(x) ((x) | 0x00000001)
#define ALIGN(x) ((x) & PAGE_FRAME)

struct pagetable;
struct pt_entry {
    vaddr_t vaddr;
    paddr_t paddr;
    int valid;
    int dirty;
    int swapped;
};
struct pagetable* pt_create();
void pt_destroy(struct pagetable* pt);
paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr, int *err);

#endif // _PT_H_
