#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>
#include "opt-A3.h"

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_bootstrap(void);


/* Prints the vm stats. Gets called before kernel exits */
void vm_shutdown(void);


/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);

#if OPT_A3
#define VM_STACKPAGES    12
#endif // OPT_A3

#endif /* _VM_H_ */
