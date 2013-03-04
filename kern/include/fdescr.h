#include "opt-A2.h"

#if OPT_A2

#define MAX_OPEN_FILES 100
#define MAX_FILENAME_LENGTH 1024   //max length of the file path
/* fdescr: a file descriptor.  Each process has a table of pointers to the 
 * system-wide table of file desriptors
 *
*/
struct fdescr {
	char fd_name[MAX_FILENAME_LENGTH];     /* the path of the file */
        volatile int fd_flags;     /* the access mode eg O_CREAT, O_RDONLY  */
	volatile int fd_offset;    /* file pointer */
	struct vnode *fd_vn;       /* points to the appropriate vnode */
	struct lock * fd_lock;     /* to ensure atomaticity of reads & writes */
};

int fd_create(const char *path, int flags, struct vnode *vn); // create fdescr
int fd_destroy(struct fdescr *fd);                              // destroy fdescr

#endif /* end of OPT_A2 */
