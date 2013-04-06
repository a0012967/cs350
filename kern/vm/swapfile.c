#include "swapfile.h"
#include "vfs.h"
#include "vm.h"
#include "vnode.h"
#include "uio.h"
#include "lib.h"
#include "types.h"
#include "uw-vmstats.h"
#include "array.h"
#include "machine/spl.h"
#include "kern/unistd.h"
#include "thread.h"
#include "curthread.h"



void swap_bootstrap(){
    // create the swapfile
	
    char sf[] = "swapfile";
	int err = vfs_open(sf, O_RDWR | O_CREAT | O_TRUNC, &swapfile);
    if (err) {
        panic("swapfile_bootstrap: error creating swapfile");
    }
	
	//create swapped pages map
	swappedpages_map = array_create();
    if (swappedpages_map == NULL) {
        panic("swapfile_bootstrap: error creating swapped pages map");
	}
	
}




int swapout(struct pt_entry *pte) {
	
	assert(pte->dirty == 1); 	
	int i, spl, found = 0;
	
	spl = splhigh();
	
	/* get offset of the page in the swapfile (if its been swapped before) */
	for(i=0; i < array_getnum(swappedpages_map); i++){
		if(pte == array_getguy(swappedpages_map, i)){
			found = 1;
			break;
		}
	}
	
	
	/* check if we are trying to swap to a full swapfile */
	if(i > MAX_SWAPPED_PAGES && !found) {
		panic("swapout: out of swapfile space");
	}
	
	
	int offset = i * PAGE_SIZE;
	int err = write_to_swapfile(pte->vaddr, offset);
	if(err) {
		return -1;
	}
	
	
	/* update the page entry */	
	pte->valid = 0;
	pte->swapped = 1;
	pte->dirty = 0;
	// TODO: figure out what to do with pte->paddr
	
	//TODO: increment swapped out stats count
	
	splx(spl);
    
	return 0;

}


void swapin( struct pt_entry *pte ) {
	
    int i, spl, offset, found = 0;
    
    spl = splhigh();
	
    //find offset of page in swapfile
    for(i=0; i<array_getnum(swappedpages_map); i++){
        if(pte == array_getguy(swappedpages_map, i)){
            found = 1;
            break;
        }
    }

    assert(found);
    
    offset = i * PAGE_SIZE;
    int err = read_from_swapfile(pte->vaddr, offset);
	if(err) {}
    //update pte
    pte->dirty = 0;
    pte->swapped = 0;
    pte->valid = 1;

    splx(spl);
}

int evict() {
	return 0;

}
 
/* read one page from swapfile at the offset into address vaddr */
int read_from_swapfile(vaddr_t vaddr, u_int32_t offset) {

    struct uio u;
    u.uio_iovec.iov_ubase = (void *)vaddr;
    u.uio_iovec.iov_len = PAGE_SIZE;
    u.uio_resid = PAGE_SIZE;
    u.uio_offset = offset;
    u.uio_segflg = UIO_SYSSPACE; //not sure about this
    u.uio_rw = UIO_READ;
    u.uio_space = NULL; //TODO: not sure about this
    int result = VOP_READ(swapfile, &u);
    return result;

}


int write_to_swapfile(vaddr_t vaddr, u_int32_t offset){

    struct uio u;
    u.uio_iovec.iov_ubase = (void *)vaddr;
    u.uio_iovec.iov_len = PAGE_SIZE;
    u.uio_resid = PAGE_SIZE;
    u.uio_offset = offset;
    u.uio_segflg = UIO_SYSSPACE; //not sure about this
    u.uio_rw = UIO_WRITE;
    u.uio_space = NULL; //TODO: not sure about this
    int result = VOP_WRITE(swapfile, &u);
	return result;
}

/*
void test_swapin() {

	struct pt_entry *pte2 = kmalloc(sizeof(struct pt_entry));
	pte2->vaddr = 0x40001;
	pte2->paddr = 0x40001;
	pte2->dirty = 1;
	pte2->valid = 1;
	pte2->swapped = 0;

	struct pt_entry *pte1 = kmalloc(sizeof(struct pt_entry));
	pte1->vaddr = 0x40003;
	pte1->paddr = 0x40003;
	pte1->dirty = 1;
	pte1->valid = 1;
	pte1->swapped = 1;
	int spl = splhigh();
	array_add(swappedpages_map, pte2 );
	array_add(swappedpages_map, pte1 );
	splx(spl);
	int i;
	for (i = 0; i< array_getnum(swappedpages_map); i++) {
		kprintf("swappedpages_map[%d]: \n", i);
		struct pt_entry *pte = array_getguy(swappedpages_map, i);
		kprintf("vaddr: %d\n", pte->vaddr);
		kprintf("paddr: %d\n", pte->paddr);
		kprintf("valid: %d\n", pte->valid);
		kprintf("dirty: %d\n", pte->dirty);
		kprintf("swapped: %d\n", pte->swapped);
	}

	kprintf("swapping out pte1\n");
	swapout(pte1);

}*/
