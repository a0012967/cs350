#include <types.h>
#include <lib.h>
#include <kern/errno.h>

#include <vm.h>
#include <elf.h>
#include <pt.h>
#include <coremap.h>

#include <array.h>
#include <queue.h>


#define IS_VALID(x) ((x) & 0x00000001)
#define SET_VALID(x) ((x) | 0x00000001)
#define ALIGN(x) ((x) & PAGE_FRAME)

struct pt_entry {
    vaddr_t vaddr;
    paddr_t paddr;
};

struct pagetable {
    struct array *entries;
};

struct pagetable* pt_create() {
    struct pagetable *pt = kmalloc(sizeof(struct pagetable));
    if (pt == NULL) {
        return NULL;
    }

    pt->entries = array_create();
    if (pt->entries == NULL) {
        kfree(pt);
        return NULL;
    }

    return pt;
}

void pt_destroy(struct pagetable *pt) {
    int i;
    for (i=0; i<array_getnum(pt->entries); i++) {
        void *ptr = array_getguy(pt->entries, i);
        if (ptr != NULL) {
            kfree(ptr);
        }
    }
    kfree(pt);
}

// copies vpn from elf to memory
paddr_t pt_pagefault_handler(vaddr_t vaddr) {
    (void)vaddr;
    paddr_t paddr = ALIGN(getppages(1));

    // TODO: read from disk

    return SET_VALID(paddr);
}

paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr) {
    struct pt_entry *pte;
    int i, found = 0;

    // align the virtual address
    vaddr = ALIGN(vaddr);

    // look for it in the pagetable
    for (i=0; i<array_getnum(pt->entries); i++) {
        pte = (struct pt_entry*)array_getguy(pt->entries, i);
        if (ALIGN(pte->vaddr) == vaddr) {
            found = 1;
            break;
        }
    }

    // if not found or valid bit is not set
    if (!found || !IS_VALID(pte->paddr)) {
        pte = kmalloc(sizeof(struct pt_entry));
        assert(pte != NULL);
        pte->vaddr = vaddr;
        pte->paddr = pt_pagefault_handler(vaddr);
        array_add(pt->entries, pte);
    }

    return ALIGN(pte->paddr);
}
