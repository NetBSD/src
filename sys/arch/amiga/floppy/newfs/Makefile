#	$NetBSD: Makefile,v 1.2 1994/10/26 02:05:43 cgd Exp $

#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c
MAN8=	newfs.0

MOUNT=	${.CURDIR}/../../../../../sbin/mount
CFLAGS+=-DMFS -I${MOUNT}
.PATH:	${.CURDIR}/../../../../../sbin/newfs ${MOUNT} ${.CURDIR}/../../../../../sbin/disklabel

BINDIR=/sbin
NOMAN=noman

.include <bsd.prog.mk>
