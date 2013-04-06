#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <thread.h>
#include <curthread.h>
#include <process.h>
#include <vm.h>
#include <addrspace.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <coremap.h>
#include <pt.h>
#include <vm.h>
#include "opt-A3.h"
#include "uw-vmstats.h"

#if OPT_A3

void
vm_bootstrap(void)
{
    coremap_bootstrap();
    vmstats_init();
}

void
vm_shutdown(void){
    _vmstats_print();
}

static int tlb_get_rr_victim() {
    int victim;
    static unsigned int next_victim = 0;
    victim = next_victim;

    if (victim < NUM_TLB) {
        _vmstats_inc(VMSTAT_TLB_FAULT_FREE);
    } else {
        _vmstats_inc(VMSTAT_TLB_FAULT_REPLACE);
    }

    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    _vmstats_inc(VMSTAT_TLB_FAULT);
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
            splx(spl);
            kill_process(-1);
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	// Assert that the address space has been set up properly.
	assert(as->as_vbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_npages2 != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);

    struct process *curprocess = get_curprocess();
    paddr = pt_lookup(curprocess->page_table, faultaddress);

	// make sure it's page-aligned 
	assert((paddr & PAGE_FRAME)==paddr);

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;

        if (as_writeable(as, faultaddress))
            elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        else
            elo = paddr | TLBLO_VALID;

		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

    // evict the victim
    int victim = tlb_get_rr_victim();
    ehi = faultaddress;
    if (as_writeable(as, faultaddress))
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
    else
        elo = paddr | TLBLO_VALID;
    DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
    TLB_Write(ehi, elo, victim);
    splx(spl);
    return 0;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
    // convert vaddr to paddr
    ungetppages((addr - MIPS_KSEG0));
}

#endif // OPT_A3
