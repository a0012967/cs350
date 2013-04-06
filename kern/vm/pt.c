#include <types.h>
#include <lib.h>
#include <kern/errno.h>

#include <vm.h>
#include <elf.h>
#include <vnode.h>
#include <uio.h>
#include <pt.h>
#include <coremap.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <process.h>

#include <array.h>
#include <queue.h>

/*
struct pt_entry {
    vaddr_t vaddr;
    paddr_t paddr;
    int valid;
    int dirty;
    int swapped;
};*/

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
        struct pt_entry *pte = (struct pt_entry*)array_getguy(pt->entries, i);
        if (pte != NULL) {
            ungetppages(ALIGN(pte->paddr));
            kfree(pte);
        }
    }
    kfree(pt);
}


static int page_read(struct vnode *v, u_int32_t offset, vaddr_t vaddr,
        size_t memsize, size_t filesize) 
{
    // kprintf("pageread: %u, %u, %u, %u\n", offset, vaddr, memsize, filesize);

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
        filesz = as->as_filesz1 - diff;
        offset = as->as_offset1 + diff;
    } else if (reg == SEG_DATA) {
        diff = vaddr - as->as_vbase2;
        filesz = as->as_filesz2 - diff;
        offset = as->as_offset2 + diff;
    } else if (reg == SEG_STCK) {
        // ignore stack segment since it doesn't live on file
        return 0;
    } else {
        // we already checked for the validity of the address before calling loadpage
        assert(0);
    }

    int ret = 0;
    if (filesz != 0) {
        ret = page_read(as->as_vnode, offset, PADDR_TO_KVADDR(paddr), PAGE_SIZE, filesz);
    }

    return ret;

}


// copies vpn from elf to memory
paddr_t pt_pagefault_handler(vaddr_t vaddr, int *err) {
    assert(*err == 0);

    paddr_t paddr = ALIGN(getppages(1));
    *err = loadpage(curthread->t_vmspace, vaddr, paddr);

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

    // address exception. kill process
    if (!as_contains(curthread->t_vmspace, vaddr)) {
        kprintf("PAGETABLE: address exception. killing process");
        kill_process(-1);
    }

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
        pte->paddr = pt_pagefault_handler(vaddr, err);
        if (*err) {
            kfree(pte);
            return 0;
        }
        array_add(pt->entries, pte);
    }

    return ALIGN(pte->paddr);
}
