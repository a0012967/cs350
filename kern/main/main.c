/*
 * Main.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <machine/spl.h>
#include <test.h>
#include <synch.h>
#include <thread.h>
#include <process.h>
#include <scheduler.h>
#include <dev.h>
#include <vfs.h>
#include <vm.h>
#include <syscall.h>
#include <version.h>
#include <curprocess.h>

// REMOVE
#include <filetable.h>
#include "opt-A0.h"

/*
 * These two pieces of data are maintained by the makefiles and build system.
 * buildconfig is the name of the config file the kernel was configured with.
 * buildversion starts at 1 and is incremented every time you link a kernel. 
 *
 * The purpose is not to show off how many kernels you've linked, but
 * to make it easy to make sure that the kernel you just booted is the
 * same one you just built.
 */
extern const int buildversion;
extern const char buildconfig[];
extern void hello();

/*
 * Copyright message for the OS/161 base code.
 */
static const char harvard_copyright[] =
    "Copyright (c) 2000, 2001, 2002, 2003\n"
    "   President and Fellows of Harvard College.  All rights reserved.\n";



void testfiletable() {
    int result = 0, err = 0;
//    struct filetable *ft = ft_create();
    struct filetable *ft = curprocess->file_table;
      assert(ft != NULL);

    struct uio u;
    struct vnode *v;

    struct file *f1 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f2 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f3 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f4 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f5 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f6 = f_create(u, v);
    assert(f1 != NULL);
    struct file *f7 = f_create(u, v);
    assert(f1 != NULL);

    result = ft_storefile(ft, f1, &err);
    assert(result != -1 && !err);
    kprintf("f1 inserted at: %d\n", result);
    result = ft_storefile(ft, f2, &err);
    assert(result != -1 && !err);
    kprintf("f2 inserted at: %d\n", result);
    result = ft_storefile(ft, f3, &err);
    assert(result != -1 && !err);
    kprintf("f3 inserted at: %d\n", result);
    result = ft_storefile(ft, f4, &err);
    assert(result != -1 && !err);
    kprintf("f4 inserted at: %d\n", result);

    kprintf("Number of files: %d\n", ft_numfiles(ft));
    kprintf("Size of filetable: %d\n", ft_getsize(ft));

    result = ft_removefile(ft, 1);
    assert(!result);
    kprintf("File removed at index: 1\n");
    result = ft_removefile(ft, 3);
    assert(!result);
    kprintf("File removed at index: 3\n");

    kprintf("Number of files: %d\n", ft_numfiles(ft));
    kprintf("Size of filetable: %d\n", ft_getsize(ft));

    result = ft_storefile(ft, f5, &err);
    assert(result != -1 && !err);
    kprintf("f5 inserted at: %d\n", result);
    result = ft_storefile(ft, f6, &err);
    assert(result != -1 && !err);
    kprintf("f6 inserted at: %d\n", result);

    kprintf("Number of files: %d\n", ft_numfiles(ft));
    kprintf("Size of filetable: %d\n", ft_getsize(ft));

    result = ft_storefile(ft, f7, &err);
    assert(result != -1 && !err);
    kprintf("f7 inserted at: %d\n", result);
    kprintf("Number of files: %d\n", ft_numfiles(ft));
    kprintf("Size of filetable: %d\n", ft_getsize(ft));

    ft_destroy(ft);
}

/*
 * Initial boot sequence.
 */
static
void
boot(void)
{
	/*
	 * The order of these is important!
	 * Don't go changing it without thinking about the consequences.
	 *
	 * Among other things, be aware that console output gets
	 * buffered up at first and does not actually appear until
	 * dev_bootstrap() attaches the console device. This can be
	 * remarkably confusing if a bug occurs at this point. So
	 * don't put new code before dev_bootstrap if you don't
	 * absolutely have to.
	 *
	 * Also note that the buffer for this is only 1k. If you
	 * overflow it, the system will crash without printing
	 * anything at all. You can make it larger though (it's in
	 * dev/generic/console.c).
	 */

	kprintf("\n");
	kprintf("OS/161 base system version %s\n", BASE_VERSION);
	kprintf("%s", harvard_copyright);
	kprintf("\n");

	kprintf("Put-your-group-name-here's system version %s (%s #%d)\n", 
		GROUP_VERSION, buildconfig, buildversion);
	kprintf("\n");

	ram_bootstrap();
	scheduler_bootstrap();
	#if OPT_A2
		process_bootstrap();
	#else
		thread_bootstrap();
	#endif // OPT_A2
	vfs_bootstrap();
	dev_bootstrap();
    #if OPT_A2
        console_files_bootstrap();
    #endif // OPT_A@
	vm_bootstrap();
	kprintf_bootstrap();

	/* Default bootfs - but ignore failure, in case emu0 doesn't exist */
	vfs_setbootfs("emu0");


	/*
	 * Make sure various things aren't screwed up.
	 */
	assert(sizeof(userptr_t)==sizeof(char *));
	assert(sizeof(*(userptr_t)0)==sizeof(char));

    #if OPT_A2
        testfiletable();
    #endif // OPT_A2

    #if OPT_A0
        hello();
    #endif /* OPT_A0 */
    
    // SIMPLE TEST
    /*
    struct process * proc = proc_create();
    struct process * proc2 = proc_create();
    struct process * proc3 = proc_create();
    
    proc_assign_thread(proc, thread_create("lu"));
    proc_assign_thread(proc2, thread_create("bu"));
    proc_assign_thread(proc3, thread_create("ru"));
    assign_pid(proc);
    print_proc_table ();
    kprintf("a1\n\n");
    
    remove_pid(1);
    print_proc_table ();
    kprintf("r1\n\n");
    
    assign_pid(proc2);
    print_proc_table ();
    kprintf("a2\n\n");
        
    remove_pid(2);
    print_proc_table ();
    kprintf("r2\n\n");
    
    assign_pid(proc3);
    print_proc_table ();
    kprintf("a3\n\n");
    
    assign_pid(proc2);
    assign_pid(proc);
    print_proc_table();*/
}

/*
 * Shutdown sequence. Opposite to boot().
 */
static
void
shutdown(void)
{

	kprintf("Shutting down.\n");
	
	vfs_clearbootfs();
	vfs_clearcurdir();
	vfs_unmountall();

	splhigh();

	scheduler_shutdown();
	thread_shutdown();
}

/*****************************************/

/*
 * reboot() system call.
 *
 * Note: this is here because it's directly related to the code above,
 * not because this is where system call code should go. Other syscall
 * code should probably live in the "userprog" directory.
 */
int
sys_reboot(int code)
{
	switch (code) {
	    case RB_REBOOT:
	    case RB_HALT:
	    case RB_POWEROFF:
		break;
	    default:
		return EINVAL;
	}

	shutdown();

	switch (code) {
	    case RB_HALT:
		kprintf("The system is halted.\n");
		md_halt();
		break;
	    case RB_REBOOT:
		kprintf("Rebooting...\n");
		md_reboot();
		break;
	    case RB_POWEROFF:
		kprintf("The system is halted.\n");
		md_poweroff();
		break;
	}

	panic("reboot operation failed\n");
	return 0;
}

/*
 * Kernel main. Boot up, then fork the menu thread; wait for a reboot
 * request, and then shut down.
 */
int
kmain(char *arguments)
{
	boot();

	menu(arguments);

	/* Should not get here */
	return 0;
}
