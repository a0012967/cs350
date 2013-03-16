#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include "opt-A2.h"


/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void mips_syscall(struct trapframe *tf)
{
    int callno;
    int32_t retval;
    int err;

    assert(curspl==0);

    callno = tf->tf_v0;

    /*
     * Initialize retval to 0. Many of the system calls don't
     * really return a value, just 0 for success and -1 on
     * error. Since retval is the value returned on success,
     * initialize it to 0 by default; thus it's not necessary to
     * deal with it except for calls that return other values, 
     * like write.
     */

    retval = 0;

    switch (callno) {
        case SYS_reboot:
            err = sys_reboot(tf->tf_a0);
            break;

        #if OPT_A2
            case SYS_open:
                err = 0;
                retval = sys_open((const char *)tf->tf_a0, (int)tf->tf_a1, &err);
                break;
            case SYS_close:
                err = 0;
                retval = sys_close((int)tf->tf_a0, &err);
                break;
            case SYS_read:
                err  = sys_read((int)tf->tf_a0, (void *)tf->tf_a1, (size_t)tf->tf_a2, &retval);
                if (err == -1) {
                    err = retval;
                }
                break;
            case SYS_fork:
                err = 0;
                retval = sys_fork(tf, &err);
                break;
            case SYS_write:
                err = 0;
                retval = sys_write((int)tf->tf_a0, (void *)tf->tf_a1, (size_t)tf->tf_a2, &err);
                if (retval < 0) {
                    retval = err;
                }
                break;
            case SYS__exit:
                sys__exit((int)tf->tf_a0);
                break;
        #endif /* OPT_A2 */

        default:
            kprintf("Unknown syscall %d\n", callno);
            err = ENOSYS;
            break;
    }


    if (err) {
        /*
         * Return the error code. This gets converted at
         * userlevel to a return value of -1 and the error
         * code in errno.
         */
        tf->tf_v0 = err;
        tf->tf_a3 = 1;      /* signal an error */
    }
    else {
        /* Success. */
        tf->tf_v0 = retval;
        tf->tf_a3 = 0;      /* signal no error */
    }

    /*
     * Now, advance the program counter, to avoid restarting
     * the syscall over and over again.
     */

    tf->tf_epc += 4;

    /* Make sure the syscall code didn't forget to lower spl */
    assert(curspl==0);
}

void md_forkentry(struct trapframe *tf)
{
    #if OPT_A2
        struct trapframe f_tf;

        f_tf.tf_vaddr = tf->tf_vaddr;	
        f_tf.tf_status = tf->tf_status;	
        f_tf.tf_cause = tf->tf_cause;	
        f_tf.tf_lo = tf->tf_lo;
        f_tf.tf_hi = tf->tf_hi;
        f_tf.tf_ra = tf->tf_ra;
        f_tf.tf_at = tf->tf_at;

        // set return code as 0
        f_tf.tf_v0 = 0;

        f_tf.tf_v1 = tf->tf_v1;
        f_tf.tf_a0 = tf->tf_a0;
        f_tf.tf_a1 = tf->tf_a1;
        f_tf.tf_a2 = tf->tf_a2;
        f_tf.tf_a3 = tf->tf_a3;
        f_tf.tf_t0 = tf->tf_t0;
        f_tf.tf_t1 = tf->tf_t1;
        f_tf.tf_t2 = tf->tf_t2;
        f_tf.tf_t3 = tf->tf_t3;
        f_tf.tf_t4 = tf->tf_t4;
        f_tf.tf_t5 = tf->tf_t5;
        f_tf.tf_t6 = tf->tf_t6;
        f_tf.tf_t7 = tf->tf_t7;
        f_tf.tf_s0 = tf->tf_s0;
        f_tf.tf_s1 = tf->tf_s1;
        f_tf.tf_s2 = tf->tf_s2;
        f_tf.tf_s3 = tf->tf_s3;
        f_tf.tf_s4 = tf->tf_s4;
        f_tf.tf_s5 = tf->tf_s5;
        f_tf.tf_s6 = tf->tf_s6;
        f_tf.tf_s7 = tf->tf_s7;
        f_tf.tf_t8 = tf->tf_t8;
        f_tf.tf_t9 = tf->tf_t9;
        f_tf.tf_k0 = tf->tf_k0;
        f_tf.tf_k1 = tf->tf_k1;
        f_tf.tf_gp = tf->tf_gp;
        f_tf.tf_sp = tf->tf_sp;
        f_tf.tf_s8 = tf->tf_s8;

        // increment program counter
        f_tf.tf_epc = tf->tf_epc + 4;

        mips_usermode(&f_tf);
    #endif // OPT_A2
}
