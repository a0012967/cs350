
mksfs.ho: \
 mksfs.c \
 $(OSTREE)/hostinclude/err.h \
 support.h \
 $(OSTREE)/hostinclude/kern/sfs.h \
 $(OSTREE)/hostinclude/hostcompat.h \
 disk.h
disk.ho: \
 disk.c \
 $(OSTREE)/hostinclude/err.h \
 support.h \
 disk.h
support.ho: \
 support.c

