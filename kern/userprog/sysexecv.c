#include <process.h>
#include <kern/errno.h>
#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <curthread.h>
#include <vnode.h>
#include <thread.h>
#include <kern/limits.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <kern/limits.h>

// check if program name is invalid
int program_invalid(const char *program, int *err) {
    if (program == NULL) {
        *err = EFAULT;
        return 1;
    }

    char foo[PATH_MAX];
	*err = copyinstr((userptr_t)program, foo, sizeof(foo), NULL);
	if (*err) {
		return 1;
	}

	if (strlen(program) <= 0) {
		*err = EINVAL;
		return 1;
	}

    return 0;
}

// check if arguments and argument pointer are invalid
int args_invalid(char **args, int *err) {
	if (args == NULL) {
        *err = EFAULT;
        return 1;
    }

    char foo[PATH_MAX];
	*err = copyinstr((userptr_t)args, foo, sizeof(foo), NULL);
	if (*err) {
		return 1;
	}
	
	int i = 0;
	while (args[i] != NULL) {
		*err = copyinstr((userptr_t)args[i], foo, sizeof(foo), NULL);
		if (*err) {
			return 1;
		}
		i++;
	}
    return 0;
}

int sys_execv(const char *program, char **args, int *err) {
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    char **argv;
    int argc;
    int result, i;
    char *prog_name;

    // check validity
	if (program_invalid(program, err)) return -1;
	if (args_invalid(args, err)) return -1;

    // copy stuff to kernel
    prog_name = kmalloc(PATH_MAX);
    copyinstr((userptr_t)program, prog_name, PATH_MAX, NULL);

    //get the number of arguments
    for (argc=0 ;args[argc] != NULL; argc++);

    argv = kmalloc(sizeof(char*));

    for(i=0; i<argc; i++) {
        size_t len = strlen(args[i]);
        len++; // add the NULL terminator
        argv[i] = kmalloc(len);
        // TODO: think about if an error occurs here
        copyinstr((userptr_t)args[i], argv[i], len, NULL);
    }

    // Terminate argv with NULL
    argv[argc] = NULL;

    // load - copied from rungprogram
    /* Open the file. */
    result = vfs_open(prog_name, O_RDONLY, &v);
    if (result) {
        *err = result;
        return -1;
    }

    // destroy the addrspace
    if (curthread->t_vmspace != NULL) {
        as_destroy(curthread->t_vmspace);
        curthread->t_vmspace = NULL;
    }

    // Create a new addrspace
    // copy the current addrspace
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        vfs_close(v);
        *err = ENOMEM;
        return -1;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        vfs_close(v);
        *err = result;
        return -1;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_vmspace */
        *err = result;
        return -1;
    }

    // copy argv to userstack
    int prev_offset = 0;
    int stack_offset = 0;
    int* arg_pointer_offset;
    vaddr_t stackptr_tracker;
    vaddr_t kernel_source;
    size_t argc_size = sizeof(int);

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
    copyout(&argc, (userptr_t)stackptr_tracker, argc_size);
    stackptr_tracker += argc_size;

    // copy all the argument pointers to user space
    for (i = 0; i < argc; i++) {
        kernel_source = stackptr + arg_pointer_offset[i] + argc_size;
        copyout(&kernel_source, (userptr_t)stackptr_tracker, sizeof(char*));
        stackptr_tracker += sizeof(char*);
    }

    // account for the null terminated argv
    stackptr_tracker += sizeof(char*);

    // copy all the argument strings to user space
    for (i = 0; i< argc; i++) {
        copyout(argv[i], (userptr_t)stackptr_tracker, strlen(argv[i]));
        stackptr_tracker += strlen(argv[i]) + ( 4 - strlen(argv[i]) % 4);
    }

    // free what we used
    kfree(arg_pointer_offset);

    // warp to user mode
    md_usermode(argc, (userptr_t)(stackptr +4), stackptr, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    *err = EINVAL;
    return -1;
}


