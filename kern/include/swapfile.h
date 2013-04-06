#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include "types.h"
#include "pt.h"

#define MAX_SWAPFILE_SIZE (9*1024*1024) // 9MB
#define MAX_SWAPPED_PAGES (MAX_SWAPFILE_SIZE / PAGE_SIZE)

struct vnode *swapfile;

// maps a swapped page to its Location/offset in the swapfile
// based on it's index in the array


int swapout(struct pt_entry *pte);
int swapin(struct pt_entry *pte);

int evict();
int write_to_swapfile(vaddr_t vaddr, u_int32_t offset);
int read_from_swapfile(vaddr_t vaddr, u_int32_t offset);

void swap_bootstrap(void);
void test_swapin();

#endif /* _SWAPFILE_H_ */

