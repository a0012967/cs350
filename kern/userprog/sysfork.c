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

// TODO: worry about synchronization problems
// TODO: worry about error codes
pid_t sys_fork(struct trapframe *tf, int *err) {
    assert(err != NULL);
    assert(*err == 0);

    int retval;
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

	new_process->parentpid = curprocess->pid;

    // copy open file information
    *err = ft_duplicate(curprocess->file_table, &(new_process->file_table));
    if (*err) {
        kfree(new_trapframe);
        ft_destroy(new_process->file_table);
        kfree(new_process);
    }

    *err = thread_fork("child_thread",       // thread name
                        new_trapframe, 0,           // arguments
                        new_thread_handler,  // function to be called
                        &new_thread);        // thread

    if (*err) {
        kfree(new_trapframe); // free trapframe
        ft_destroy(new_process->file_table);
        kfree(new_process);
        goto fail;
    }

    // set the process reference of the thread to new_process' pid
    new_thread->pid = new_process->pid;

    // set thread of new process
    new_process->p_thread = new_thread;

    // set the retval to new_process' pid
    retval = new_process->pid;

    // return 1 for now (must return pid)
    return retval;

fail:
    assert(*err);
    return -1;
}
