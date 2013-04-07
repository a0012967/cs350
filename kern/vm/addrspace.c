#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <process.h>
#include <processtable.h>
#include <pt.h>
#include <vm.h>
#include <uio.h>
#include <addrspace.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <vnode.h>
#include <coremap.h>
#include "opt-A3.h"
#include "uw-vmstats.h"

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace * as_create(void) {
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

#if OPT_A3
    as->as_vnode = NULL;
	as->as_vbase1 = 0;
	as->as_npages1 = 0;
    as->as_filesz1 = 0;
    as->as_offset1 = 0;
    as->as_flags1 = 0;

	as->as_vbase2 = 0;
	as->as_npages2 = 0;
    as->as_filesz2 = 0;
    as->as_offset2 = 0;
    as->as_flags2 = 0;

    as->as_stackpbase = 0;
#endif // OPT_A3

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret) {
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

#if OPT_A3
    (void)old;
#else
	(void)old;
#endif // OPT_A3
	
	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
    VOP_DECREF(as->as_vnode);
    ungetppages(as->as_stackpbase);
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
#if OPT_A3
	int i, spl;

	(void)as;

	spl = splhigh();

    _vmstats_inc(VMSTAT_TLB_INVALIDATE);
    // invalidate TLB
	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
#else
	(void)as;  // suppress warning until code gets written
#endif // OPT_A3
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int as_define_region(struct addrspace *as, 
                     struct vnode *v,
				     vaddr_t vaddr, 
                     u_int32_t filesz,
                     u_int32_t offset,
                     size_t sz,
                     int readable, 
                     int writeable,
                     int executable)
{
#if OPT_A3
    u_int32_t flags = 0;
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

    // set flags
    if (readable)   flags |= SEG_RD;
    if (writeable)  flags |= SEG_WR;
    if (executable) flags |= SEG_EX;

    // increment references to file
    VOP_INCREF(v);

	if (as->as_vbase1 == 0) {
        as->as_vnode = v;
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
        as->as_filesz1 = filesz;
        as->as_offset1 = offset;
        as->as_flags1 = flags;
		return 0;
	}

	if (as->as_vbase2 == 0) {
        as->as_vnode = v;
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
        as->as_filesz2 = filesz;
        as->as_offset2 = offset;
        as->as_flags2 = flags;
		return 0;
	}

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");

#else
	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
#endif // OPT_A3
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
#if OPT_A3
    as->as_stackpbase = getppages(VM_STACKPAGES);
    if (as->as_stackpbase == 0) {
        return ENOMEM;
    }
#else
	(void)as;
#endif // OPT_A3
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
    (void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
#if OPT_A3
    assert(as->as_stackpbase != 0);
#else
	(void)as;
#endif

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

#if OPT_A3
int as_contains(struct addrspace *as, vaddr_t vaddr) {
    vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
    stackbase = USERSTACK - VM_STACKPAGES * PAGE_SIZE;
    stacktop = USERSTACK;

    if (vaddr >= vbase1 && vaddr < vtop1)
        return SEG_TEXT;
    else if (vaddr >= vbase2 && vaddr < vtop2)
        return SEG_DATA;
    else if (vaddr >= stackbase && vaddr < stacktop)
        return SEG_STCK;

    return 0;
}

int as_writeable(struct addrspace *as, vaddr_t vaddr) {
    u_int32_t seg;
    u_int32_t flags;

    seg = as_contains(as, vaddr);

    if (seg == SEG_TEXT)
        flags = as->as_flags1;
    else if (seg == SEG_DATA)
        flags = as->as_flags2;
    else if (seg == SEG_STCK)
        flags = SEG_WR;
    else
        return 0;

    return flags & SEG_WR;
}
#endif // OPT_A3
