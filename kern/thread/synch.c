/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <process.h>
#include <curthread.h>
#include <machine/spl.h>
#include <syscall.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}
	
    #if OPT_A1
        lock->held = NULL;
        lock->wait_queue = q_create(MAXTHREADS);
    #else
    #endif /* OPT_A1 */
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

    #if OPT_A1
        // since we're putting threads to sleep on the lock's address
        int spl = splhigh();
        assert(thread_hassleepers(lock)==0);
        splx(spl);

        // something wrong with code if destroy is called before lock_release
        assert(lock->held == NULL);
        q_destroy((struct queue *)lock->wait_queue);
    #else
    #endif /* OPT_A1 */
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
    #if OPT_A1
        // Write this
        int spl;
        assert(lock != NULL);
        assert(in_interrupt==0);

        spl = splhigh();
        if (lock->held != NULL) {
            int err = q_addtail((struct queue *)lock->wait_queue, curthread);
            if (err) {
                splx(spl);
                kill_process(0);
            }
            thread_sleep(curthread);
        }
        lock->held = curthread;
        splx(spl);
    #else
        (void)lock;  // suppress warning until code gets written
    #endif /* OPT_A1 */
}

void
lock_release(struct lock *lock)
{
    #if OPT_A1
        // Write this
        assert(lock != NULL);
        assert(lock_do_i_hold(lock));

        int spl;
        struct thread *t;

        spl = splhigh();

        // if there are threads waiting, wake it up
        // this thread will be the first to access the critical section
        if (!q_empty((struct queue *)lock->wait_queue)) {
            t = (struct thread *)q_remhead((struct queue *)lock->wait_queue);
            lock->held = t;
            thread_wakeup(t);
        }
        else {
            lock->held = NULL;
        }

        splx(spl);
    #else
        (void)lock;  // suppress warning until code gets written
    #endif /* OPT_A1 */
}

int
lock_do_i_hold(struct lock *lock)
{
    #if OPT_A1
        return lock->held == curthread;
    #else
        (void)lock;  // suppress warning until code gets written
        return 1;    // dummy until code gets written
    #endif /* OPT_A1 */
}

////////////////////////////////e///////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
    #if OPT_A1
        cv->wait_queue = q_create(MAXTHREADS);
    #else
    #endif /* OPT_A1 */
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

    #if OPT_A1
        int spl = splhigh();
        assert(thread_hassleepers(cv)==0);
        splx(spl);
        q_destroy((struct queue *)cv->wait_queue);
    #else
    #endif /* OPT_A1 */
	
	kfree(cv->name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
    #if OPT_A1
        int spl;
        assert(cv != NULL && lock != NULL);
        assert(lock_do_i_hold(lock));
        lock_release(lock);
        spl = splhigh(); // disable interrupts on thread functions
        int err = q_addtail((struct queue *)cv->wait_queue, curthread);
        assert(err == 0);
        thread_sleep(curthread);
        splx(spl);
        lock_acquire(lock);
    #else
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
    #endif /* OPT_A1 */
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    #if OPT_A1
        int spl;
        assert(cv != NULL && lock != NULL);
        assert(lock_do_i_hold(lock));
        spl = splhigh();  // disable interrupts
        if (!q_empty((struct queue *)cv->wait_queue)) {
            struct thread *t = (struct thread *)q_remhead((struct queue *)cv->wait_queue);
            thread_wakeup(t);
        }
        splx(spl);
    #else
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
    #endif /* OPT_A1 */
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
    #if OPT_A1
        int spl;
        assert(cv != NULL && lock != NULL);
        assert(lock_do_i_hold(lock));
        spl = splhigh(); // disable interrupts
        while (!q_empty((struct queue *)cv->wait_queue)) {
            struct thread *t = (struct thread *)q_remhead((struct queue *)cv->wait_queue);
            thread_wakeup(t);
        }
        splx(spl);
    #else
        (void)cv;    // suppress warning until code gets written
        (void)lock;  // suppress warning until code gets written
    #endif /* OPT_A1 */
}

