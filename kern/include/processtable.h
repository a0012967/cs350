#ifndef _PROCESSTABLE_H_
#define _PROCESSTABLE_H_

// NOTE: all arguments passed to function call must be valid, else it will crash the system
// if it's not valid, something's wrong with how you're computing your arguments
// this should be the case to let kernel programmers know that they're being stupid

#include <types.h>

struct process;

// bootstraps processtable
void processtable_bootstrap();

// inserts process into processtable. returns index where process was inserted
int processtable_insert(struct process *p);

// BEWARE! This function just removes the reference to the process in the table
// It doesn't deallocate the process. YOU HAVE TO CALL destroy the process yourself
void processtable_remove(pid_t pid);

// returns process with the given pid. If pid is invalid or process does not exist, panics
struct process* processtable_get(pid_t pid);

// For debugging purposes only
int processtable_getnum(); // size of table - NULL(freed) entries
int processtable_getsize(); // size of table

#endif // _PROCESSTABLE_H_
