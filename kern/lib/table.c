#include <table.h>
#include <types.h>
#include <lib.h>
#include <array.h>
#include <linkedlist.h>
#include <kern/errno.h>



/***********
 * Private
 ************/

struct table {
    struct array *array;
    struct linkedlist *slots;
};

int get_first_slot(struct table *t) {
    if (ll_size(t->slots) == 0) {
        return -1;
    }

    int *ptr = (int *)ll_pop_front(t->slots);
    int ret = *ptr; // copy the value

    // free memory allocated to ptr
    kfree(ptr); 

    return ret;
}

/***********
 * Public 
 ************/

struct table* tab_create() {
    struct table *t = kmalloc(sizeof(struct table));
    if (t == NULL) {
        return NULL;
    }

    t->array = array_create();
    if (t->array == NULL) {
        kfree(t);
        return NULL;
    }

    t->slots = ll_create();
    if (t->slots == NULL) {
        array_destroy(t->array);
        kfree(t);
        return NULL;
    }

    return t;
}

void tab_destroy(struct table *t) {
    assert(t != NULL);

    // free allocated data stored in linked list
    while(!ll_empty(t->slots)) {
        void *ptr = ll_pop_back(t->slots);
        kfree(ptr);
    }

    ll_destroy(t->slots);
    array_destroy(t->array);
    kfree(t);
}

int tab_add(struct table *t, void *ptr, int *err) {
    assert(t != NULL);
    assert(*err == 0);
    int result;
    int slot = get_first_slot(t);

    // if there is NO free slot, append
    if (slot == -1) {
        result = array_add(t->array, ptr); 
        // signal failure
        if (result) {
            *err = result;
            return -1;
        }
        return array_getnum(t->array) - 1;
    }

    // if there is free slot, insert
    array_setguy(t->array, slot, ptr); 
    return slot;
}

// returns 0 if successful, -1 otherwise
// sets NULL to pointer in table at the given index
// saves the index as free in linked list
int tab_remove(struct table *t, int index) {
    assert(t != NULL);

    int result;
    int size = array_getnum(t->array);
    assert(index >= 0 && index < size);

    // TODO: worry about shrinking the array and freeing up memory used

    // replace pointer stored as NULL
    array_setguy(t->array, index, NULL);

    // store the index to free slots
    // this is gonna be freed in destroy or when the slot is used
    int *i_ptr = kmalloc(sizeof(int));
    *i_ptr = index;
    result = ll_push_back(t->slots, i_ptr);

    return result;
}

// returns the ptr at the given index
// returns NULL when ptr has been removed
void* tab_getguy(struct table *t, int index) {
    assert(t != NULL);
    return array_getguy(t->array, index);
}

// DUPLICATES TABLE (SOFT COPY)
// returns error code on failure
int tab_duplicate(struct table *t, struct table **new_t) {
    assert(t != NULL);
    assert(*new_t != NULL);

    struct table *n_t;
    struct array *n_arr;
    struct linkedlist *n_ll;
    void *ptr;
    int i, size, err=0;

    n_t = *new_t;
    n_arr = n_t->array;
    n_ll = n_t->slots;
    size = tab_getsize(t);

    // copy array
    err = array_preallocate(n_arr, size);
    if (err) {
        goto fail;
    }

    // soft copy is okay in this case since they are shared
    for (i=0; i<size; i++) {
        ptr = array_getguy(t->array, i);
        array_add(n_arr, ptr);
    }

    // SOFT-COPY LINKED LIST
    struct linkedlist *temp_ll = ll_copy(t->slots); 
    if (temp_ll == NULL) {
        err = ENOMEM;
        goto fail;
    }
    // HARD-COPY LINKED LIST
    while (!ll_empty(temp_ll)) {
        int *tmp_ptr = (int*)ll_pop_front(temp_ll);
        int *ptr = kmalloc(sizeof(int));
        if (ptr == NULL) {
            err = ENOMEM;
            ll_destroy(temp_ll);
            goto fail;
        }
        *ptr = *tmp_ptr;
        ll_push_back(n_ll, ptr);
    }
    // DESTROY THE TEMPORARY LINKED LIST
    ll_destroy(temp_ll);

    if (err) {
        goto fail;
    }

    return 0;

fail:
    assert(err);
    return err;
}

int tab_getsize(struct table *t) {
    assert(t != NULL);
    return array_getnum(t->array);
}

// WARNING! DON'T USE THIS FOR NOW! USE THIS FOR THE SAKE OF DEBUGGING!
// returns number of entries in table - number of holes
int tab_getnum(struct table *t) {
    assert(t != NULL);
    // numbers of entry in table - free slots
    return array_getnum(t->array) - ll_size(t->slots);
}
