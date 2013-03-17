#include <processtable.h>
#include <process.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
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

int processtable_insert(struct process *p) {
    assert(p != NULL);
    int result;
    int err = 0;

    lock_acquire(pt_lock);
        result = tab_add(process_table, p, &err);
        if (result == -1 || err != 0) {
            panic("PROCESSTABLE: Cannot add process to processtable\n");
        }
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
            panic("PROCESSTABLE: Process does not exist in process table at the given pid: %d\n", pid);
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

