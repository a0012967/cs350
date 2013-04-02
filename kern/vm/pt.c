#include <types.h>
#include <lib.h>
#include <kern/errno.h>

#include <vm.h>
#include <pt.h>

#include <array.h>
#include <queue.h>

// HELPERS
u_int32_t out_index(vaddr_t vaddr);
u_int32_t in_index(vaddr_t vaddr);

typedef u_int32_t pte_t; // page table entry
typedef u_int32_t page_t;

struct pagetable {
    struct array *pt_out;
}

struct pagetable* pt_create() {
    int i, rc=0;

    struct pagetable *pt = kmalloc(sizeof(struct pagetable));
    if (pt == NULL) {
        return NULL;
    }

    pt->pt_out= array_create();
    if (pt->pt_out== NULL) {
        kfree(pt);
        return NULL;
    }

    rc = array_setsize(pt->pt_out, PT_OUT_SIZE);
    if (rc != 0) {
        array_destroy(pt->pt_out);
        kfree(pt);
        return NULL;
    }

    for (i=0; i<PT_OUT_SIZE; i++) {
        array_setguy(pt->pt_out, i, NULL);
    }

    return pt;
}

void pt_destroy(struct pagetable *pt) {
    assert(pt != NULL);
    int i;
    for (i=0; i<PT_OUT_SIZE; i++) {
        void *ptr = array_getguy(pt->pt_out, i);
        if (ptr != NULL) {
            kfree(ptr);
        }
    }
    array_destroy(pt->pt_entries);
    kfree(pt);
}

pte_t pt_loadpage(page_t* pt_in, vaddr_t vaddr) {
    u_int32_t in_i;
    pte_t pte;

    // TODO: load page to physical memory;
    // we know how to translate virtual addresses to virtual addresses
    // pte = getPaddr(vaddr);

    in_i = in_index(vaddr);
    *(pt_in + in_i) = pte;

    return pte;
}

paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr) {
    // TODO: vpn not in PT? would fail in addrspace functions
    // TODO: a bunch of other stuff as well

    u_int32_t i, out_i, in_i;
    pte_t pte;
    page_t* pt_in;

    // get indices
    out_i = out_index(vaddr);
    in_i = in_index(vaddr);

    // get inside pagetable
    pt_in = (page_t*)array_getguy(pt->pt_out, out_i);

    // if inside table is not yet created, create it
    if (pt_in == NULL) {
        pt_in = kmalloc(sizeof(page_t) * PT_IN_SIZE);
        assert(pt_in != NULL);

        // zero-out
        for (i=0; i<PT_IN_SIZE; i++) {
            *(pt_in+i) = 0;
        }
    }

    pte = *(pt_in+in_i);

    // PAGE FAULT
    if ((pte & PTE_VALID) == 0) {
        pte = pt_loadpage(pt_in, vaddr);
    }

    return pte;
}

u_int32_t out_index(vaddr_t vaddr) {
    // right shift makes leftmost bit 0 for unsigned ints
    return vaddr >> 22;
}

u_int32_t in_index(vaddr_t vaddr) {
    // 0 out other bits and do a shift
    return (ret & IN_MASK) >> 12;
}
