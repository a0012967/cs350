#ifndef _PT_H_
#define _PT_H_

#define IS_VALID(x) ((x) & 0x00000001)
#define SET_VALID(x) ((x) | 0x00000001)
#define ALIGN(x) ((x) & PAGE_FRAME)

struct pagetable;
struct pagetable* pt_create();
void pt_destroy(struct pagetable* pt);
paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr);

#endif // _PT_H_
