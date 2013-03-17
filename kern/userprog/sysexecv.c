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

int
sys_execv(const char *progname, char **args, int *retval)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	if(progname == NULL)
	{
		*retval = -1;
		return EFAULT;
	}

	char *fname = (char *)kmalloc(1024);
	size_t size;
	copyinstr((userptr_t)progname,fname,1024,&size);	

	int i = 0;
	while(args[i] != NULL)
  		i++;
  	int argc = i;

	char **argv;
	argv = (char **)kmalloc(sizeof(char*));

	// Copy in all the argumens in args
	size_t arglen;
	for(i = 0; i < argc; i++) {
		int len = strlen(args[i]);
		len++;
		argv[i]=(char*)kmalloc(len);
		copyinstr((userptr_t)args[i], argv[i], len, &arglen);
  	}
  	//Null terminate argv
  	argv[argc] = NULL;


	/* Open the file. */
	result = vfs_open(fname, 0, &v);
	if (result) {
		*retval = -1;
		return result;
	}
	//destroy the address space for a new loadelf
	if(curthread->t_vmspace != NULL)
	{
		as_destroy(curthread->t_vmspace);
		curthread->t_vmspace=NULL;
	}
	/* We should be a new thread. */

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		*retval = -1;
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		vfs_close(v);
		*retval = -1;
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		*retval = -1;
		return result;
	}

	//set up the arguments in the user stack


  		
	//copy the contents to stack
	unsigned int pstack[argc];
	//size_t arglen;
	for(i = argc-1; i >= 0; i--) {
		int len = strlen(argv[i]);
		int shift = (len%4);
		if(shift == 0)
			shift = 4;
		stackptr = stackptr - (len + shift);
		copyoutstr(argv[i], (userptr_t)stackptr, len, &arglen);
		pstack[i] = stackptr;
	}

	pstack[argc] = (int)NULL;
	for(i = argc-1; i >= 0; i--)
	{
		stackptr = stackptr - 4;
		copyout(&pstack[i] ,(userptr_t)stackptr, sizeof(pstack[i]));
	}
	//kprintf("in execv: %s",(char *)stackptr_ptr+4);	
	//null terminate stack
	//int term = 0;
	//memcpy((userptr_t)stackptr_trav, &term, sizeof(term));
	//memcpy((userptr_t)stackptr_ptr, &term, sizeof(term));
	DEBUG(DB_EXEC, "DEBUG EXEC %s", progname);
	//args = pt;
	//copyout(pt,(userptr_t)stackptr,sizeof(pt));

	*retval = 0;	
	kfree(argv);
	/* Warp to user mode. */
	md_usermode(argc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr, entrypoint);

	return EINVAL;
}








