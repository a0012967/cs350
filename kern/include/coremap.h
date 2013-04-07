#ifndef _COREMAP_H_
#define _COREMAP_H_

// paddr -> the physical address of the page
// use -> is the address in use by a page?
// size -> the size of the page loaded on that address
//          0 -> completely free
//         -1 -> index is part of a contiguous block

struct coremap_entry {
    paddr_t paddr;
    u_int32_t use;
    int size;
};

void coremap_bootstrap();
paddr_t getppages(unsigned long npages);
void ungetppages(paddr_t paddr);

#endif // _COREMAP_H_

