# $NetBSD: Makefile,v 1.3 2001/10/05 01:17:31 jmc Exp $
# Build a smaller mount (i.e. for boot media)

PROG=	mount
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/mount

SRCS=	mount.c vfslist.c

CPPFLAGS+= -DNO_MOUNT_PROGS -I${SRCDIR}
.if ${MACHINE_ARCH} != "sparc64"
DBG= -Os
.endif
LDSTATIC?=-static
WARNS?= 1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
