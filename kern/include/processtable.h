#ifndef _PROCESSTABLE_H_
#define _PROCESSTABLE_H_

struct process;

// bootstraps processtable
void processtable_bootstrap();
// inserts process into processtable. returns index where process was inserted
int processtable_insert(struct process *p);
// BEWARE! This function just removes the reference to the process in the table
// It doesn't deallocate the process. YOU HAVE TO CALL destroy the process yourself
void processtable_remove(int fd);

// For debugging purposes only
int pt_getnum(); // size of table - NULL(freed) entries
int pt_getsize(); // size of table

#endif // _PROCESSTABLE_H_
