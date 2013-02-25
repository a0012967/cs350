#include <syscall.h>

#if OPT_A2
    int sys_read(int fd, void *buf, size_t buflen) {
        (void)fd;
        (void)buf;
        (void)buflen;
        return 0;
    }
#endif /* OPT_A2 */
