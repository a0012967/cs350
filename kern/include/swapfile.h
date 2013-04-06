#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include "types.h"
#include "pt.h"
#define MAX_SWAPFILE_SIZE (9*1024*1024) // 9MB
#define MAX_SWAPPED_PAGES (MAX_SWAPFILE_SIZE / PAGE_SIZE)

struct vnode *swapfile;

// maps a swapped page to its Location/offset in the swapfile
// based on it's index in the array
struct array *swappedpages_map;
/*
struct pt_entry {
	paddr_t paddr;
	vaddr_t vaddr;
	int valid;
	int swapped;
	int dirty;	
};
*/
void swapin();  
int swapout(struct pt_entry *pte);   // swaps the page pte to disk/swapfile
int evict();
int write_to_swapfile(vaddr_t vaddr, u_int32_t offset);
int read_from_swapfile(vaddr_t vaddr, u_int32_t offset);
void swap_bootstrap(void);
void test_swapin();
#endif /* _SWAPFILE_H_ */
