# $NetBSD: Makefile,v 1.2 2001/08/20 12:19:45 martin Exp $
# Build a smaller mount (i.e. for boot media)

PROG=	mount
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/mount

SRCS=	mount.c vfslist.c

CPPFLAGS+= -DNO_MOUNT_PROGS -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
COPTS+= -Os
.endif
LDSTATIC?=-static
WARNS?= 1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
