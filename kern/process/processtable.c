#include <processtable.h>
#include <process.h>
#include <thread.h>
#include <curthread.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <lib.h>

static struct table *process_table;
static struct lock *pt_lock;

void processtable_bootstrap() {
    process_table = tab_create();
    if (process_table == NULL) {
        panic("PROCESSTABLE: Cannot create process table\n");
    }

    pt_lock = lock_create("pt_lock");
    if (pt_lock == NULL) {
        panic("PROCESSTABLE: Cannot create process table lock\n");
    }
}

int processtable_insert(struct process *p, int *err) {
    assert(p != NULL && err != NULL);
    assert(*err == 0);
    int result;
    int num_entries;

    lock_acquire(pt_lock);
        num_entries = tab_getnum(process_table);
        result = tab_add(process_table, p, err);
    lock_release(pt_lock);

    return result;
}

void processtable_remove(int fd) {
    lock_acquire(pt_lock);
        int result = tab_remove(process_table, fd);
        if (result == -1) {
            panic("PROCESSTABLE: Cannot remove process: %d in processtable\n", fd);
        }
    lock_release(pt_lock);
}

struct process * processtable_get(pid_t pid) {
    if (pid < 0 || pid >= processtable_getsize())
		return NULL;
		
    struct process *p;

    lock_acquire(pt_lock);
        p = (struct process*)tab_getguy(process_table, pid);
        if (p == NULL) {
        	lock_release(pt_lock);
            return NULL;
            //panic("PROCESSTABLE: Process does not exist in process table at the given pid: %d\n", pid);
        }
    lock_release(pt_lock);

    return p;
}

int processtable_getnum() {
    int ret;
    lock_acquire(pt_lock);
        ret = tab_getnum(process_table);
    lock_release(pt_lock);
    return ret;
}

int processtable_getsize() {
    int ret;
    lock_acquire(pt_lock);
        ret = tab_getsize(process_table);
    lock_release(pt_lock);
    return ret;
}
