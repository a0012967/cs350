#ifndef _PROCESS_H_
#define _PROCESS_H_

/*
 * Process used storing pid and thread
 *
 * Contains: 
 *     - pid_t for the process id.
 *     - a thread pointer to the process's thread
 * Functions
 *     process_create        - allocates a new process.
 *                             Returns NULL on error
 *     process_assign_thread - assigns the supplied thread
 *								             to the supplied process
 */

struct process;

struct process * process_create();
//void process_assign_thread(struct process * proc, struct thread * thread);


/*
 * Process table used for pid management
 *
 * Contains: 
 *     - an array of processes where a process with pid i
 *       is at index i of the array.
 *     - pid_t to represent next available pid, in the array
 *       (does not account for holes in the array - that is managed by
 *        the freed_pids linked list)
 *     - a linked list to keep track of freed pids that can be used again
 * Functions
 *     process_table_create  - allocates a new process table.
 *                             Returns NULL on error
 *     process_assign_thread - assigns the supplied thread
 *								             to the supplied process
 */

struct process_table;

struct process_table * process_table_create();

#endif // _PROCESS_H_



