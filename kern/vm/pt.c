#include <types.h>
#include <lib.h>
#include <kern/errno.h>

#include <vm.h>
#include <elf.h>
#include <vnode.h>
#include <uio.h>
#include <pt.h>
#include <coremap.h>
#include <swapfile.h>
#include <machine/tlb.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <process.h>
#include "uw-vmstats.h"
#include <array.h>
#include <linkedlist.h>


struct pagetable {
    struct array *entries;
    struct linkedlist *fifo;
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

    pt->fifo = ll_create();
    if (pt->fifo == NULL) {
        array_destroy(pt->entries);
        kfree(pt);
        return NULL;
    }

    return pt;
}

void pt_destroy(struct pagetable *pt) {
    int i;

    // free page table entries inside array
    for (i=0; i<array_getnum(pt->entries); i++) {
        struct pt_entry *pte = (struct pt_entry*)array_getguy(pt->entries, i);
        if (pte != NULL) {
            ungetppages(ALIGN(pte->paddr));
            kfree(pte);
        }
    }

    // empty queue
    while (!ll_empty(pt->fifo)) {
        ll_pop_front(pt->fifo);
    }

    array_destroy(pt->entries);
    ll_destroy(pt->fifo);
    kfree(pt);
}

static int page_read(struct vnode *v, u_int32_t offset, vaddr_t vaddr,
        size_t memsize, size_t filesize) 
{
    struct uio ku;
    int result;
    size_t fillamt;

	if (filesize > memsize) {
		filesize = memsize;
	}

	DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n", (unsigned long) filesize, (unsigned long) vaddr);

    mk_kuio(&ku, (void*)vaddr, memsize, offset, UIO_READ);
    ku.uio_resid = filesize;

    result = VOP_READ(v, &ku);
    if (result) {
        return result;
    }
	
    if (ku.uio_resid != 0) {
		// short read; problem with executable?
		kprintf("ELF: short read on segment - file truncated?\n");
		return ENOEXEC;
    }

	// Fill the rest of the memory space (if any) with zeros
	fillamt = memsize - filesize;
	if (fillamt > 0) {		
		DEBUG(DB_EXEC, "ELF: Zero-filling %lu more bytes\n", (unsigned long) fillamt);
		ku.uio_resid += fillamt;
		result = uiomovezeros(fillamt, &ku);		
	}
	
	return result;
}

static int loadpage(struct addrspace *as, vaddr_t vaddr, paddr_t paddr) {
    u_int32_t reg, diff, offset, filesz;

    vaddr = ALIGN(vaddr);
    paddr = ALIGN(paddr);

    // get which region vaddr lies in
    reg = as_contains(as, vaddr);

    if (reg == SEG_TEXT) {
        diff = vaddr - as->as_vbase1;
        filesz = as->as_filesz1 > diff ? as->as_filesz1 - diff : 0;
        offset = as->as_offset1 + diff;
    }
    else if (reg == SEG_DATA) {
        diff = vaddr - as->as_vbase2;
        filesz = as->as_filesz2 > diff ? as->as_filesz2 - diff : 0;
        offset = as->as_offset2 + diff;
    }
    else {
        // we already checked for the validity of the address before calling loadpage
        assert(0);
    }

	vmstats_inc(VMSTAT_ELF_FILE_READ);
	vmstats_inc(VMSTAT_PAGE_FAULT_DISK);
    return page_read(as->as_vnode, offset, PADDR_TO_KVADDR(paddr), PAGE_SIZE, filesz);
}

static struct pt_entry* pt_get_fifo_victim(struct pagetable *pt) {
    return (struct pt_entry*)ll_pop_front(pt->fifo);
}

static paddr_t page_replace(struct pagetable *pt) {
    int i;

    struct pt_entry *pte = pt_get_fifo_victim(pt);
    swapout(pte);
    assert(ALIGN(pte->paddr) > 0);
    assert(IS_SWAPPED(pte->paddr));

    /*if (IS_DIRTY(pte->paddr)) {
        kprintf("swapout(pte)\n");
        swapout(pte);
    }
    else {
        kprintf("pte->paddr = SET_INVALID(pte->paddr)\n");
        pte->paddr = SET_INVALID(pte->paddr);
    }*/

    // invalidate tlb entry
    i = TLB_Probe(pte->vaddr, pte->paddr);
    if (i >= 0)
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);

    return ALIGN(pte->paddr);
}

static void zero_out_page(paddr_t paddr) {
    struct uio ku;
    paddr = ALIGN(paddr); assert(paddr > 0);
    mk_kuio(&ku, (void*)PADDR_TO_KVADDR(paddr), PAGE_SIZE, 0, UIO_READ);
    ku.uio_resid = PAGE_SIZE;
    int result = uiomovezeros(PAGE_SIZE, &ku);
    assert(!result);
}

// forces to get a free page. puts replace page into swapfile
static void force_free_page(struct pagetable *pt) {
    paddr_t p = page_replace(pt);
    assert(ALIGN(p) > 0);
    zero_out_page(p);
    ungetppages(p);
}

// copies vpn from elf to memory
paddr_t pt_pagefault_handler(struct pagetable *pt, vaddr_t vaddr, int *err) {
    assert(*err == 0);

    paddr_t paddr = ALIGN(getppages(1));

    // no memory
    if (paddr == 0) {
        paddr = page_replace(pt);
        zero_out_page(paddr);
    }
    else  {
        // load from elf
        *err = loadpage(curthread->t_vmspace, vaddr, paddr);
    }

    if (*err)
        return 0;

    return SET_VALID(paddr);
}

paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr, int *err) {
    assert(*err == 0);

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

    if (found) {
        if (IS_VALID(pte->paddr)) {
            // TLB miss for a page in memory
            vmstats_inc(VMSTAT_TLB_RELOAD);
        }
        else {
            assert(IS_SWAPPED(pte->paddr));
            *err = swapin(pte); assert(*err == 0);
			vmstats_inc(VMSTAT_PAGE_FAULT_DISK);

            while (ll_push_back(pt->fifo, pte)) {
                force_free_page(pt);
            }
        }

    } else {
        pte = kmalloc(sizeof(struct pt_entry));
        while (pte == NULL) {
            force_free_page(pt);
            pte = kmalloc(sizeof(struct pt_entry));
        }

        pte->vaddr = vaddr;
        pte->paddr = pt_pagefault_handler(pt, vaddr, err);

        if (*err) {
            assert(0);
            kfree(pte);
            return 0;
        }

        while (array_add(pt->entries, pte)) {
            force_free_page(pt);
        }

        while (ll_push_back(pt->fifo, pte)) {
            force_free_page(pt);
        }
    }

    pte->paddr = ALIGN(pte->paddr);
    pte->paddr = SET_VALID(pte->paddr);
    return ALIGN(pte->paddr);
}
