#include <systemfiletable.h>
#include <types.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <synch.h>
#include <file.h>

struct systemfiletable {
    struct file *head;
    struct file *tail;
    int count;
};

static struct systemfiletable *sft;
static struct lock *sft_lock;

void systemfiletable_bootstrap() {
    sft = kmalloc(sizeof(struct systemfiletable));
    if (sft == NULL) {
        panic("SYSTEMFILETABLE: Can't create system file table\n");
    }
    sft_lock = lock_create("sft_lock");
    if (sft_lock == NULL) {
        panic("SYSTEMFILETABLE: Can't create system file table lock\n");
    }

    sft->head = NULL;
    sft->tail = NULL;
    sft->count = 0;
}

int systemft_insert(struct file *f) {
    assert(f != NULL);
    // f has to be newly-created
    assert(f->prev == NULL && f->next == NULL && f->count == 0);

    lock_acquire(sft_lock);
        if (sft->count >= OPEN_MAX) {
            assert(0); // assert for now
        }

        // 0 current entries in sytem-wide filetable
        if (sft->tail == NULL) {
            sft->head = f;
            sft->tail = f;
        }
        // 1 current entry in system-wide filetable
        else if (sft->head == sft->tail) {
            assert(sft->head->prev == NULL);
            sft->head->next = f;
            f->prev = sft->head;
            sft->tail = f;
        }
        // 2 or more current entry in system-wide filetable
        else {
            assert(sft->tail != NULL);
            sft->tail->next = f;
            f->prev = sft->tail;
            sft->tail = f;
        }

        f->count = 1;
        sft->count += 1; // increment systemfiletable count
    lock_release(sft_lock);

    return 0;
}

int systemft_remove(struct file *f) {
    assert(f != NULL);
    assert(f->count > 0);

    if (sft->count <= 0)
        return -1;

    lock_acquire(sft_lock);
        f->count -= 1;
        if (f->count == 0) {
            if (f->prev == NULL && f->next == NULL) {
                assert(sft->head == f && sft->tail == f);
                sft->head = NULL;
                sft->tail = NULL;
            }
            else {
                if (f->next == NULL) {
                    assert(f == sft->tail);
                    sft->tail = sft->tail->prev;
                    sft->tail->next = NULL;
                }
                else if (f->prev == NULL) {
                    assert(f == sft->head);
                    sft->head = f->next;
                    sft->head->prev = NULL;
                }
                else {
                    f->prev->next = f->next;
                    f->next->prev = f->prev;
                }
            }
            f_destroy(f);
            sft->count -= 1;
        }
    lock_release(sft_lock);

    return 0;
}

int systemft_update(struct file *f) {
    assert(f != NULL);
    lock_acquire(sft_lock);
        f->count += 1;
    lock_release(sft_lock);
    return 0;
}

int systemft_getcount() {
    int ret;
    lock_acquire(sft_lock);
        ret = sft->count;
    lock_release(sft_lock);
    return ret;
}

// iterates to count number of elements and returns it
int systemft_iter_ct() {
    struct file *curr;
    int ret;
    lock_acquire(sft_lock);
        if (sft->head == NULL)
            return 0;
        curr = sft->head;
        ret = 1;
        while(curr != sft->tail) {
            ret++;
            curr = curr->next;
        }
    lock_release(sft_lock);
    return ret;
}

/*
void runtests() {
    struct file *f1 = f_create(0, 0, NULL);
    struct file *f2 = f_create(0, 0, NULL);
    struct file *f3 = f_create(0, 0, NULL);
    struct file *f4 = f_create(0, 0, NULL);
    struct file *f5 = f_create(0, 0, NULL);
    struct file *f6 = f_create(0, 0, NULL);
    systemft_insert(f1);
    systemft_insert(f2);
    systemft_insert(f3);
    systemft_insert(f4);
    systemft_insert(f5);
    systemft_insert(f6);
    kprintf("Size of SFT: %d\n", sft->count);

    systemft_remove(f1);
    kprintf("Size of SFT: %d\n", sft->count);
    systemft_remove(f6);
    kprintf("Size of SFT: %d\n", sft->count);
    systemft_remove(f3);
    kprintf("Size of SFT: %d\n", sft->count);

    kprintf("Counted entries: %d\n", systemft_iter_ct());
}
*/
