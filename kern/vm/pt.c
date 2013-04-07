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

#include <array.h>
#include <linkedlist.h>

struct pagetable {
    struct array *pt_out;
    struct linkedlist *fifo;
};

struct pagetable* pt_create() {
    int i, err = 0;

    struct pagetable *pt = kmalloc(sizeof(struct pagetable));
    if (pt == NULL) {
        return NULL;
    }

    pt->pt_out = array_create();
    if (pt->pt_out == NULL) {
        kfree(pt);
        return NULL;
    }

    err = array_setsize(pt->pt_out, N_OUT);
    if (err) {
        array_destroy(pt->pt_out);
        kfree(pt);
        return NULL;
    }

    for (i=0; i<N_OUT; i++) {
        array_setguy(pt->pt_out, i, NULL);
    }

    pt->fifo = ll_create();
    if (pt->fifo == NULL) {
        array_destroy(pt->pt_out);
        kfree(pt);
        return NULL;
    }

    return pt;
}

void pt_destroy(struct pagetable *pt) {
    int i, j;

    struct array *in_arr;
    struct pt_entry *pte;

    for (i=0; i<N_OUT; i++) {
        in_arr = (struct array*)array_getguy(pt->pt_out, i);
        if (in_arr != NULL) {
            for (j=0; j<N_IN; j++) {
                pte = (struct pt_entry*)array_getguy(in_arr, i);
                if (pte != NULL) {
                    if (!SET_SWAPPED(pte->paddr)) {
                        ungetppages(ALIGN(pte->paddr));
                        kfree(pte);
                    }
                }
            }
            array_destroy(in_arr);
        }
    }

    while (!ll_empty(pt->fifo)) {
        ll_pop_front(pt->fifo);
    }

    array_destroy(pt->pt_out);
    ll_destroy(pt->fifo);
    kfree(pt);
}

static u_int32_t out_index(vaddr_t vaddr) {
    u_int32_t ret = vaddr >> 22;
    return ret;
}

static u_int32_t in_index(vaddr_t vaddr) {
    u_int32_t ret = (vaddr & IN_MASK) >> 12;
    return ret;
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
    else if (reg == SEG_STCK) {
        // ignore stack segment since it doesn't live in elf file
        return 0;
    }
    else {
        // we already checked for the validity of the address before calling loadpage
        assert(0);
    }

    return page_read(as->as_vnode, offset, PADDR_TO_KVADDR(paddr), PAGE_SIZE, filesz);
}

static struct pt_entry* pt_get_fifo_victim(struct pagetable *pt) {
    return (struct pt_entry*)ll_pop_front(pt->fifo);
}

static paddr_t page_replace(struct pagetable *pt) {
    int i;

    struct pt_entry *pte = pt_get_fifo_victim(pt);
    swapout(pte);

    /*
    if (IS_DIRTY(pte->paddr)) {
        kprintf("swapping out\n");
        swapout(pte);
    }
    else
        pte->paddr = SET_INVALID(pte->paddr);
        */

    // invalidate tlb entry
    i = TLB_Probe(pte->vaddr, 0);
    if (i >= 0) {
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }


    return ALIGN(pte->paddr);
}

paddr_t pt_pagefault_handler(struct pagetable *pt, vaddr_t vaddr, int *err) {
    assert(*err == 0);

    paddr_t paddr = ALIGN(getppages(1));
    // no memory
    if (paddr == 0) {
        paddr = page_replace(pt);
    }
    // load from elf
    else  {
        *err = loadpage(curthread->t_vmspace, vaddr, paddr);
    }

    if (*err)
        return 0;

    return SET_VALID(paddr);
}

paddr_t pt_lookup(struct pagetable *pt, vaddr_t vaddr, int *err) {
    assert(*err == 0);

    struct pt_entry *pte;
    struct array *in_arr;
    int i;

    // calculate indices
    u_int32_t out_i = out_index(vaddr);
    u_int32_t in_i = in_index(vaddr);

    // align the virtual address
    vaddr = ALIGN(vaddr);

    // address exception. kill process
    if (!as_contains(curthread->t_vmspace, vaddr)) {
        kprintf("PAGETABLE: address exception. killing process");
        kill_process(-1);
    }

    // look for it in the pagetable
    in_arr = (struct array*)array_getguy(pt->pt_out, out_i);

    if (in_arr == NULL) {
        int tmp_err;
        paddr_t tmp_paddr;

        in_arr = array_create();
        while (in_arr == NULL) {
            kprintf("248\n");
            tmp_paddr = page_replace(pt);
            ungetppages(ALIGN(tmp_paddr));
            in_arr = array_create();
        }

        tmp_err = array_setsize(in_arr, N_IN);
        while (tmp_err) {
            kprintf("256\n");
            tmp_paddr = page_replace(pt);
            ungetppages(ALIGN(tmp_paddr));
            tmp_err = array_setsize(in_arr, N_IN);
        }

        for (i=0; i<N_IN; i++) {
            array_setguy(in_arr, i, NULL);
        }

        array_setguy(pt->pt_out, out_i, in_arr);
    }

    pte = (struct pt_entry*)array_getguy(in_arr, in_i);

    if (pte != NULL) {
        if (!IS_VALID(pte->paddr)) {
            // it should've been swapped out
            assert(IS_SWAPPED(pte->paddr));
            swapin(pte);

            assert(ALIGN(pte->paddr) > 0);
            while (ll_push_back(pt->fifo, pte)) {
                kprintf("279\n");
                paddr_t p = page_replace(pt);
                kprintf("p is: %u\n", p);
                ungetppages(ALIGN(p));
            }
        }
        assert(!IS_SWAPPED(pte->paddr));
    }
    else {
        pte = kmalloc(sizeof(struct pt_entry));
        int x=0;
        while (pte == NULL) {
            kprintf("290: %d\n", x);
            paddr_t p = page_replace(pt);
            ungetppages(ALIGN(p));
            pte = kmalloc(sizeof(struct pt_entry));
            x++;
        }
        assert(pte != NULL);


        pte->vaddr = ALIGN(vaddr);
        pte->paddr = pt_pagefault_handler(pt, vaddr, err);
        pte->paddr = SET_VALID(pte->paddr);
        //kprintf("pte contents: %u, %u\n", pte->vaddr, pte->paddr);
        if (*err) {
            assert(0);
            kfree(pte);
            return 0;
        }

        array_setguy(in_arr, in_i, pte);

        assert(ALIGN(pte->paddr) > 0);
        while(ll_push_back(pt->fifo, pte)) {
            paddr_t p = page_replace(pt);
            ungetppages(ALIGN(p));
            kprintf("316\n");
        }
    }

    return ALIGN(pte->paddr);
}

