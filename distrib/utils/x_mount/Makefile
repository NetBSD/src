# $NetBSD: Makefile,v 1.1 2001/02/18 06:15:49 tsutsui Exp $
# Build a smaller mount (i.e. for boot media)

PROG=	mount
MKMAN=	no

SRCDIR= ${.CURDIR}/../../../sbin/mount

SRCS=	mount.c vfslist.c

CPPFLAGS+= -DNO_MOUNT_PROGS -I${SRCDIR}
COPTS+= -Os
LDSTATIC?=-static
WARNS?= 1

all: ${PROG}

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
