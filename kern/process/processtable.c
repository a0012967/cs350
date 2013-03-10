#include <processtable.h>
#include <process.h>
#include <table.h>
#include <synch.h>
#include <kern/errno.h>
#include <lib.h>
#include <types.h>

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


int processtable_getnum() {
    lock_acquire(pt_lock);
        return tab_getnum(process_table);
    lock_release(pt_lock);
}

int processtable_getsize() {
    lock_acquire(pt_lock);
        return tab_getsize(process_table);
    lock_release(pt_lock);
}

