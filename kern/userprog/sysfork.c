#include <syscall.h>
#include <curthread.h>
#include <thread.h>
#include <process.h>
#include <processtable.h>
#include <thread.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <addrspace.h>
#include <filetable.h>
#include <array.h>
#include <kern/errno.h>


// set up to be called by new thread during fork
static void new_thread_handler(void *tf, unsigned long dummy) {
    (void)dummy;

    struct trapframe new_tf;
    struct trapframe *old_tf = tf;

    new_tf = *old_tf;
    kfree(tf);

    md_forkentry(&new_tf);
}

pid_t sys_fork(struct trapframe *tf, int *err) {
    assert(err != NULL);
    assert(*err == 0);

    int retval;
    struct process *curprocess = processtable_get(curthread->pid);
    struct process *new_process;
    struct thread *new_thread;
    struct trapframe *new_trapframe;

    // copy trapframe
    new_trapframe = kmalloc(sizeof(struct trapframe));
    *new_trapframe = *tf; // copy trapframe

    // create new process
    new_process = p_create();
    if (new_process == NULL) {
        kfree(new_trapframe);
        *err = ENOMEM;
        goto fail;
    }

    // copy open file information
    *err = ft_duplicate(curprocess->file_table, &(new_process->file_table));
    if (*err) {
        kfree(new_trapframe);
        ft_destroy(new_process->file_table);
        kfree(new_process);
    }

    *err = thread_fork("child_thread",       // thread name
                        new_trapframe, 0,    // arguments
                        new_thread_handler,  // function to be called
                        &new_thread,         // thread
                        new_process);        // reference to process

    if (*err) {
        kfree(new_trapframe); // free trapframe
        ft_destroy(new_process->file_table);
        kfree(new_process);
        goto fail;
    }

    // set the retval to new_process' pid
    retval = new_process->pid;

	// add the new proc's pid to the current proc's array of children 
	array_add(curprocess->p_childrenpids, new_process->pid);

    // return 1 for now (must return pid)
    return retval;

fail:
    assert(*err);
    return -1;
}
