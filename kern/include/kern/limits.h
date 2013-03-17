#ifndef _KERN_LIMITS_H_
#define _KERN_LIMITS_H_

/* Longest filename (without directory) not including null terminator */
#define NAME_MAX  (255)

/* Longest full path name */
#define PATH_MAX   (1024)

// Max number of files in per-process filetable
#define PROC_FILES_MAX (60)

// Max number of files in system-wide filetable
#define SYS_FILES_MAX (1024)

#endif /* _KERN_LIMITS_H_ */
