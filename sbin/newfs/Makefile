#	from: @(#)Makefile	8.2 (Berkeley) 3/27/94
#	$Id: Makefile,v 1.7 1994/06/08 19:30:24 mycroft Exp $

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c
MAN8=	newfs.0

MOUNT=	${.CURDIR}/../mount
CFLAGS+=-DMFS -I${MOUNT}
.PATH:	${MOUNT} ${.CURDIR}/../disklabel

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8 newfs.8 mfs.8

.include <bsd.prog.mk>
