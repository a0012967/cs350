#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <thread.h>
#include <curthread.h>
#include <process.h>
#include <vm.h>
#include <addrspace.h>
#include <swapfile.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <coremap.h>
#include <pt.h>
#include <vm.h>
#include <swapfile.h>
#include "opt-A3.h"
#include "uw-vmstats.h"

#if OPT_A3

void vm_bootstrap(void) {
    coremap_bootstrap();
    vmstats_init();
}

void vm_shutdown(void) {
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

static 
void 
tlb_replace(struct addrspace *as, vaddr_t faultaddress, paddr_t paddr) {
    // find victim to replace
    u_int32_t victim = tlb_get_rr_victim();
    u_int32_t ehi = faultaddress;
    u_int32_t elo = paddr | TLBLO_VALID;

    // mark as dirtiable if writeable
    if (as_writeable(as, faultaddress)) {
        elo |= TLBLO_DIRTY;
    }

    DEBUG(DB_VM, "vm_tlb: 0x%x -> 0x%x\n", faultaddress, paddr);

    // evict the victim
    TLB_Write(ehi, elo, victim);
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
    _vmstats_inc(VMSTAT_TLB_FAULT);

	struct addrspace *as;
	paddr_t paddr;
	u_int32_t ehi, elo;
	int i, spl, err = 0;

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
        splx(spl);
		return EFAULT;
	}

	// Assert that the address space has been set up properly.
	assert(as->as_vbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_npages2 != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);

    // look in current process page table for frame number
    paddr = pt_lookup(get_curprocess()->page_table, faultaddress, &err);
    if (err) {
        splx(spl);
        return err;
    }

	// make sure it's page-aligned 
	assert((paddr & PAGE_FRAME)==paddr);

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);

		if (elo & TLBLO_VALID){
			continue;
		}

		ehi = faultaddress;
        elo = paddr | TLBLO_VALID;

        if (as_writeable(as, faultaddress)) {
            elo |= TLBLO_DIRTY;
        }

		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

    // TLB full
    tlb_replace(as, faultaddress, paddr);

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
