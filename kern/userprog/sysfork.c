#include <syscall.h>
#include <process.h>
#include <curprocess.h>
#include <processtable.h>
#include <thread.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <addrspace.h>
#include <filetable.h>


// set up to be called by new thread during fork
void new_thread_handler(void *tf, unsigned long dummy) {
    (void)dummy;
    md_forkentry((struct trapframe *)tf);
}

// TODO: worry about synchronization problems
// TODO: worry about error codes
pid_t sys_fork(struct trapframe *tf, int *err) {
    assert(err != NULL);
    assert(*err == 0);

    int retval;
    int spl;
    struct addrspace *old_as;
    struct addrspace *new_as;
    struct process *new_process;
    struct thread *new_thread;
    struct filetable *new_ft;

    // copy address space
    old_as = curprocess->p_thread->t_vmspace;
    new_as = as_create();
    if (new_as == NULL) {
        // TODO: error code
        assert(0);
        goto fail;
    }
	new_as->as_vbase1 = old_as->as_vbase1;
	new_as->as_pbase1 = old_as->as_pbase1;
	new_as->as_npages1 = old_as->as_npages1;
	new_as->as_vbase2 = old_as->as_vbase2;
	new_as->as_pbase2 = old_as->as_pbase2;
	new_as->as_npages2 = old_as->as_npages2;
	new_as->as_stackpbase = old_as->as_stackpbase;

    // copy open file information
    new_ft = ft_duplicate(curprocess->file_table, err);
    if (new_ft == NULL) {
        as_destroy(new_as);
        goto fail;
    }

    // disable interrupts
    spl = splhigh();

    // create new process and new thread
    new_process = p_create();
    if (new_process == NULL) {
        // TODO: error code
        assert(0);
        goto fail;
    }

    *err = thread_fork("child_thread",       // thread name
                        tf, 0,               // arguments
                        new_thread_handler,  // function to be called
                        &new_thread);        // thread
    if (*err) {
        as_destroy(new_as);
        ft_destroy(new_ft);
        // TODO: error code
        assert(0);
        goto fail;
    }

    // set address space of new thread
    new_thread->t_vmspace = new_as;

    // activate address space
    as_activate(new_thread->t_vmspace);

    // set thread of new process
    new_process->p_thread = new_thread;

    // set the retval to new_process' pid
    retval = new_process->pid;

    // enable interrupts
    splx(spl);

    // return 1 for now (must return pid)
    return retval;

fail:
    assert(*err);
    return -1;
}
