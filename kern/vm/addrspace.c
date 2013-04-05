#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <coremap.h>
#include <addrspace.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <array.h>
#include "opt-A3.h"
#include "uw-vmstats.h"

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

#define TEMPSTACKPAGES    12

struct addrspace * as_create(void) {
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

#if OPT_A3
    as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
    as->as_flags1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
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
	newas->as_vbase1 = old->as_vbase1;
	newas->as_npages1 = old->as_npages1;
	newas->as_vbase2 = old->as_vbase2;
	newas->as_npages2 = old->as_npages2;

	if (as_prepare_load(newas)) {
		as_destroy(newas);
		return ENOMEM;
	}

	assert(newas->as_pbase1 != 0);
	assert(newas->as_pbase2 != 0);
	assert(newas->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(newas->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(newas->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(newas->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		TEMPSTACKPAGES*PAGE_SIZE);
#else
	(void)old;
#endif // OPT_A3
	
	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	
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
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
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

    if (readable)   flags |= AS_SEG_RD;
    if (writeable)  flags |= AS_SEG_WR;
    if (executable) flags |= AS_SEG_EX;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
        as->as_flags1 = flags;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
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
	assert(as->as_stackpbase == 0);
	as->as_stackpbase = getppages(TEMPSTACKPAGES);
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
#if OPT_A3
	(void)as;
#else
	(void)as;
#endif // OPT_A3
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
#if OPT_A3
	assert(as->as_stackpbase != 0);
#else
	(void)as;
#endif // OPT_A3

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

#if OPT_A3
int isWriteable(struct addrspace *as, vaddr_t vaddr) {
    vaddr_t vbase1, vtop1, vbase2, vtop2;
    u_int32_t flags;

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;

    if (vaddr >= vbase1 && vaddr < vtop1)
        flags = as->as_flags1;
    else if (vaddr >= vbase2 && vaddr < vtop2)
        flags = as->as_flags2;
    else
        return 1;

    return 1;

    if (flags & AS_SEG_WR)
        return 1;

    return 0;
}

#endif // OPT_A3

