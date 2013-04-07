#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <thread.h>
#include <curthread.h>
#include <coremap.h>
#include <vm.h>
#include <addrspace.h>
#include <uio.h>
#include <machine/spl.h>
#include <machine/tlb.h>


static u_int32_t cm_size;
static paddr_t firstaddr;
static paddr_t lastaddr;
static paddr_t freeaddr; 
static u_int32_t cm_bootstrapped = 0;
static struct coremap_entry ** cm;


void coremap_bootstrap() {
    u_int32_t i = 0;
    struct coremap_entry *cm_entry;

    // figure out how much RAM we have to work with
    ram_getsize(&firstaddr, &lastaddr);
    cm_size = (lastaddr - firstaddr) / PAGE_SIZE;

    // create core map    
    cm = kmalloc(sizeof (struct coremap_entry*) * cm_size);
    if (cm == NULL) {
        panic("Coremap could not be allocated");
        return;
    }

    // instantiate all possible entries on the coremap
    for (i = 0; i < cm_size; i++) {
        cm_entry = kmalloc(sizeof ( struct coremap_entry ));
        if (cm_entry == NULL) {
            panic("Coremap entry could not be allocated");
            return;
        }
        cm_entry->size = 0;
        cm_entry->use = 0;
        cm_entry->paddr = (PAGE_SIZE * i) + firstaddr;
        cm[i] = cm_entry;
    }

    // update our addresses to figure out 
    ram_getsize(&freeaddr, &lastaddr);

    // make all phys addr before our free addr fixed so that 
    // structures needed for the OS are ensured not to be removed
    for (i = 0; i < cm_size; i++) {
        if (cm[i]->paddr >= freeaddr) {
            break;
        }
        cm[i]->size = -1;
        cm[i]->use = 1;
    }

    cm_bootstrapped = 1;
}

paddr_t getppages(unsigned long npages) {
	int spl;
	paddr_t addr;

	spl = splhigh();

    // do this only if the coremap hasnt been initialized yet
    // ======================================================
	if (cm_bootstrapped == 0) {
	    addr = ram_stealmem(npages);
	    splx(spl);
	    return addr;
	}
    // ======================================================

    u_int32_t cont_count = npages; // counts contiguous pages
    u_int32_t cont_block_index; // index of the contiguous block
    u_int32_t i = 0;

    // iterate through the coremap to find a contiguous block
    // to hold npages
    for (i = 0; i <= cm_size; i++) {
        if (cont_count == 0) {
            cont_block_index = i - npages;
            break;
        }
        else if (cm[i]->use == 0) {
            cont_count--;
        }
        else if (i == cm_size -1) {
        	// TODO what if we dont find a block big enough?
            splx(spl);
            return 0;        
        }
        else {
            cont_count = npages;
        }
    }

    if (cont_count != 0) {
        splx(spl);
        return 0;
    }
	
	// set the first page of the block
    addr = cm[cont_block_index]->paddr;
    cm[cont_block_index]->use = 1;
    cm[cont_block_index]->size = npages;

	struct uio ku;
	mk_kuio(&ku, (void*)PADDR_TO_KVADDR(addr), PAGE_SIZE*npages, 0, UIO_READ);
	ku.uio_resid = PAGE_SIZE*npages;
	uiomovezeros(PAGE_SIZE*npages,&ku);

	// set the "tail" of the block of pages to be in use
	for (i = cont_block_index+1; i < cont_block_index + npages; i++) {
	    cm[i]->use = 1;
	    cm[i]->size = -1;
	}
	
	splx(spl);
	return addr;
}


void ungetppages(paddr_t paddr) {
    int spl;
    u_int32_t i = 0;

    spl = splhigh();

    // calculate index of the addr
    u_int32_t index = (paddr - firstaddr) / PAGE_SIZE;

    // make the phys addr available on the coremap
    for (i = index + cm[index]->size -1; i >= index; i--) {
        cm[i]->use = 0;
        cm[i]->size = 0;
    }

    splx(spl);
}
