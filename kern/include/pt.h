#ifndef _PT_H_
#define _PT_H_

/*
 * Two-level page table
 * FIFO-based page-replacement algorithm
 * First 10 bit is the outer page table
 */

// HAVE TO CHANGE THESE WHEN YOU CHANGE PAGE_SIZE

#define PTE_VALID       (0x1)
#define OUT_MASK        (0xffc00000)    // top 10 bits
#define IN_MASK         (0x003ff000)    // next 10 bits
#define OFFSET_MASK     (0x00000FFF)    // final 12 bits
#define PT_OUT_SIZE     (1024)          // 2^10 = 1024
#define PT_IN_SIZE      (1024)
#define SWAP_SPACE      (9*1024*1024)


struct pagetable;
struct pagetable* pt_create();
void pt_destroy(struct pagetable* pt);
// handles page faults
paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr);


#endif // _PT_H_
