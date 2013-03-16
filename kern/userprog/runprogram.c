/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include <process.h>
#include <curprocess.h>
#include "opt-A2.h"

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, int argc, char ** argv)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

    // TODO: remove this from here. let fork do its job properly
    #if OPT_A2
        struct process *p = p_create();
        p_assign_thread(p, curthread);
        curprocess = p;
        console_files_bootstrap();
    #endif /*OPT_A2*/

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}
	
    #if OPT_A2
        int i;
        int prev_offset = 0;
        int stack_offset = 0;
        int* arg_pointer_offset;
        vaddr_t stackptr_tracker;
        vaddr_t kernel_source;
        size_t argc_size = sizeof(int);
        
        // set argument array to be null terminated
        argv[argc] = NULL;
        
        // - find the offset from the argument pointer for each argument
        // - the offsets are aligned by 4, since that is the size of 
        //   each char pointer
        // - the total stack offset is kept track of
        arg_pointer_offset = kmalloc(argc_size * (argc+1));
        arg_pointer_offset[0] = argc_size*(argc+1);
        for(i = 1; i <= argc; i++) {
            prev_offset = arg_pointer_offset[i-1];
            arg_pointer_offset[i] = prev_offset + 
                (4 - strlen(argv[i-1])%4) + strlen(argv[i-1]);
            stack_offset += arg_pointer_offset[i] - prev_offset;
        }
        
        // since the stack offset at the moment only contains the total number
        // of characters in the argument strings, we need to add the argument 
        // pointers, the argv null terminator, and argc to the stack offset
        stack_offset += (argc+1)*sizeof(char*) + argc_size;
        
        // move the stack pointer to the offseted location, then use the 
        // stack pointer tracker to copy the stack from bottom up
        stackptr -= stack_offset;
        stackptr_tracker = stackptr;
        
        // copy the number of arguments to user space
        copyout(&argc, stackptr_tracker, argc_size);
        stackptr_tracker += argc_size;
        
        // copy all the argument pointers to user space
        for (i = 0; i < argc; i++) {
            kernel_source = stackptr + arg_pointer_offset[i] + argc_size;
            copyout(&kernel_source, stackptr_tracker, sizeof(char*));
            stackptr_tracker += sizeof(char*);
        }
        
        // account for the null terminated argv
        stackptr_tracker += sizeof(char*);
        
        // copy all the argument strings to user space
        for (i = 0; i< argc; i++) {
            copyout(argv[i], stackptr_tracker, strlen(argv[i]));
            stackptr_tracker += strlen(argv[i]) + ( 4 - strlen(argv[i]) % 4);
        }
        
        // free what we used
        kfree(arg_pointer_offset);
        
        // warp to user mode
        md_usermode(argc, (stackptr +4), stackptr, entrypoint);
    #else
	    /* Warp to user mode. */
	    md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
		        stackptr, entrypoint);
	#endif // OPT_A2
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");
	return EINVAL;
}
