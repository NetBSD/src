# $NetBSD: Makefile,v 1.4 2001/12/12 00:05:13 tv Exp $
# Build a smaller mount (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/mount

PROG=		mount
NOMAN=		# defined

SRCS=		mount.c vfslist.c
CPPFLAGS+=	-DNO_MOUNT_PROGS -I${SRCDIR}
WARNS?=		1

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
