#	$NetBSD: Makefile,v 1.2 1997/05/06 20:45:10 gwr Exp $

PROG=	mount_ffs
SRCS=	mount_ffs.c getmntopts.c
MAN=	mount_ffs.8

MLINKS=	mount_ffs.8 mount_ufs.8
LINKS=  ${BINDIR}/mount_ffs ${BINDIR}/mount_ufs

MOUNT=	${.CURDIR}/../mount
CFLAGS+= -I${MOUNT}
.PATH.c: ${MOUNT}

.include <bsd.prog.mk>
