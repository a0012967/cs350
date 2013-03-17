#include <process.h>
#include <kern/errno.h>
#include <types.h>
#include <lib.h>
#include <kern/unistd.h>
#include <curthread.h>
#include <vnode.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
int sys_execv(char * program, char **args, int *err) {

    int argc = 0;
    int padby = 0; // remember to reset to 0 each time
    int actual;
    char **argv;    
	int i, j;
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;
    actual = 0;
    struct addrspace *curaddrspace, *newaddrspace;    


    // TODO: error checking!
    if (program == NULL) {
        *err = ENOENT;
        return -1;
    }

 
    
   /*******************************
       COPY ARGS INTO KSPACE
    ******************************/
    
    //get the number of arguments
    while (args[argc] != NULL){
        argc++;    
    }

    argv = kmalloc(sizeof(char *) * (argc + 2 )); // + 1 for the NULL terminating entry, +1 for program
    
    // copy in program pointers
    // need to check valid program name!
    argv[0] = program;
  
/*  *err = copyin(program, argv, sizeof(char*));
     if (*err){ return -1;}*/

     // copy in the argument pointers
    argv++;
     *err = copyin((userptr_t)args, argv, (argc)*sizeof(char*));
    argv--;
    if (*err) { return -1;}

    argv[argc+1] = NULL;
    // TODO: make sure argv starts at an address divisible by 4
   

    //copy in the program string
   /* *err = copyinstr( program, 
                      argv[0], 
                      sizeof(char)*strlen(program),
                      &actual
                      );   
    if (*err) { return -1; }*/

   // copy the argument strings from args into kernel space
    
    for(i = 0; i < argc; i++) {
         //copy the arguments in 
         int j = 0;
         while (args[i][j] != '\0') { j++;} // get the length of the arg
            //find how much padding is needed to align to 4 bytes
            padby = 4 - (j % 4);
            if (padby == 4) {  // already aligned
                padby = 0;
            }
       //     argv[i] = kmalloc(sizeof(char)*(j+padby));
        
            *err = copyinstr(args[i], argv[i+1], sizeof(char)*j, &actual);
            //TODO: figure out of this is necessary           
             while (padby > 0) {
               //  j++;
                 argv[i+1][j] = '\0';
                 j++;
                 padby--;
             }

        
    }


	   /*******************************
    LOAD ELF - COPIED FROM RUNPROGRAM
     *******************************/


    /* Open the file. */
    result = vfs_open(program, O_RDONLY, &v);
    if (result) {
        *err = result;
        return -1;
    }

    /* Create a new address space. */
    // copy the current addrspace
    curaddrspace = curthread->t_vmspace;
    // make a new address space
    newaddrspace = as_create();
    // switch to new address space
    curthread->t_vmspace = newaddrspace;;
    if (curthread->t_vmspace==NULL) {
        vfs_close(v);
        *err = ENOMEM;
        return -1;
    }
    //TODO: do we destroy old addrspace?

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
        *err = result;
        return -1;
    }
    
    //not sure about this
    as_destroy(curaddrspace);




    /**********************************
        COPY ARGV ONTO USERSTACK
     **********************************/
	    int prev_offset = 0;
        int stack_offset = 0;
        int* arg_pointer_offset;
        vaddr_t stackptr_tracker;
        vaddr_t kernel_source;
        size_t argc_size = sizeof(int);

		// add one to argc since we've added program to argv
		argc++;

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


		/* md_usermode does not return */
		panic("md_usermode returned\n");
		return EINVAL;
		

              
} 













