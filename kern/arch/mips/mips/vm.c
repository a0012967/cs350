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
#include "opt-A3.h"
#include "uw-vmstats.h"
#if OPT_A3
#define VM_STACKPAGES    12

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

/*
static u_int32_t notdirtiable(u_int32_t x) {
    return x & (0xffffff - TLBLO_DIRTY);
}
*/

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    _vmstats_inc(VMSTAT_TLB_FAULT);

	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;

	spl = splhigh();

	faultaddress &= PAGE_FRAME;

	// DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

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

	/* Assert that the address space has been set up properly. */
	assert(as->as_vbase1 != 0);
	assert(as->as_pbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_pbase2 != 0);
	assert(as->as_npages2 != 0);
	assert(as->as_stackpbase != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - VM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		splx(spl);
		return EFAULT;
	}

	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;

        if (isWriteable(as, faultaddress)) {
            elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        }
        else {
            elo = (paddr & (~TLBLO_DIRTY)) | TLBLO_VALID;
        }

		// DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
		return 0;
	}

    assert(0);

    // evict the victim
    int victim = tlb_get_rr_victim();
    ehi = faultaddress;
    if (isWriteable(as, faultaddress)) {
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        kprintf("writeable\n");
    }
    else {
        elo = (paddr & (~TLBLO_DIRTY)) | TLBLO_VALID;
        kprintf("not writeable\n");
    }
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
