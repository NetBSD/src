PROG= ../bin/iscsi-target
SRCS= disk.c iscsi-target.c target.c iscsi.c util.c parameters.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CPPFLAGS+= -I${.CURDIR}/../include
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
NOMAN= # defined
WARNS=4

.include <bsd.prog.mk>
