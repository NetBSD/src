#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.1.2.2 1994/07/30 19:30:49 chopps Exp $

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c
MAN8=	newfs.0

MOUNT=	${.CURDIR}/../../../../../sbin/mount
CFLAGS+=-DMFS -I${MOUNT}
.PATH:	${.CURDIR}/../../../../../sbin/newfs ${MOUNT} ${.CURDIR}/../../../../../sbin/disklabel

BINDIR=/sbin
NOMAN=noman

.include <bsd.prog.mk>
