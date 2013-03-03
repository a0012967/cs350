#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#define MAXFILES 1024

struct vnode;

void ft_bootstrap();
int storefile(struct vnode* file);
int removefile(int fd);
struct vnode* getfile(int fd);

#endif /* _FILETABLE_H_ */
