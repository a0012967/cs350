#include <swapfile.h>
#include <types.h>
#include <lib.h>
#include <vfs.h>
#include <vm.h>
#include <vnode.h>
#include <uio.h>
#include <uw-vmstats.h>
#include <array.h>
#include <machine/spl.h>
#include <kern/unistd.h>
#include <thread.h>
#include <curthread.h>
#include <pt.h>


static paddr_t *swappedpages_map;
static u_int32_t count;

void swap_bootstrap() {
    int err = 0;
    char *sf = NULL;

    sf = kstrdup("swapfile");
	err = vfs_open(sf, O_RDWR | O_CREAT | O_TRUNC, &swapfile);
    assert(!err);
	
	swappedpages_map = kmalloc(MAX_SWAPPED_PAGES * sizeof(paddr_t));
    assert(swappedpages_map);

    count = 0;
}

int swapout(struct pt_entry *pte) {
    assert(IS_DIRTY(pte->paddr));

    u_int32_t i, spl, offset, err = 0, found = 0;
	spl = splhigh();
	
    paddr_t pfn = ALIGN(pte->paddr);

	/* get offset of the page in the swapfile (if its been swapped before) */
	for (i=0; i<count; i++) {
		if (pfn == swappedpages_map[i]) {
			found = 1;
			break;
		}
	}	
	
	/* check if we are trying to swap to a full swapfile */
	if (count == MAX_SWAPPED_PAGES && !found) {
		panic("swapout: out of swapfile space");
	}
	
    if (found) {
        offset = i * PAGE_SIZE;
    }
    else {
        swappedpages_map[count] = pfn;
        offset = count * PAGE_SIZE;
        count++;
    }

	err = write_to_swapfile(PADDR_TO_KVADDR(pfn), offset);
    assert(!err);

	// update page table entry
    // turn off all other bits and set it as swapped
    pte->paddr = SET_SWAPPED(ALIGN(pte->paddr));
	
	//TODO: increment swapped out stats count
	
	splx(spl);
	return err;
}


int swapin(struct pt_entry *pte) {
    assert(IS_SWAPPED(pte->paddr));

    u_int32_t i, spl, offset, err = 0, found = 0;
    spl = splhigh();
	
    paddr_t pfn = ALIGN(pte->paddr);

    // find offset of page in swapfile
    for (i=0; i<count; i++) {
        if (pfn == swappedpages_map[i]) {
            found = 1;
            break;
        }
    }

    assert(found);

	/* read page from swapfie */
    offset = i * PAGE_SIZE;
    err = read_from_swapfile(PADDR_TO_KVADDR(pfn), offset);
    assert(!err);

	// update page table entry
    // turn off all other bits and set it as valid
    pte->paddr = SET_VALID(ALIGN(pte->paddr));

    splx(spl);
    return err;
}

int read_from_swapfile(vaddr_t vaddr, u_int32_t offset) {
    struct uio u;
    u.uio_iovec.iov_ubase = (void *)vaddr;
    u.uio_iovec.iov_len = PAGE_SIZE;
    u.uio_resid = PAGE_SIZE;
    u.uio_offset = offset;
    u.uio_segflg = UIO_SYSSPACE;
    u.uio_rw = UIO_READ;
    u.uio_space = NULL;
    return VOP_READ(swapfile, &u);
}

int write_to_swapfile(vaddr_t vaddr, u_int32_t offset) {
    struct uio u;
    u.uio_iovec.iov_ubase = (void *)vaddr;
    u.uio_iovec.iov_len = PAGE_SIZE;
    u.uio_resid = PAGE_SIZE;
    u.uio_offset = offset;
    u.uio_segflg = UIO_SYSSPACE;
    u.uio_rw = UIO_WRITE;
    u.uio_space = NULL;
    return VOP_WRITE(swapfile, &u);
}

int evict() {
	return 0;
}

