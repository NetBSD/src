#	$NetBSD: Makefile,v 1.18 2001/07/29 09:59:12 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c ffs_bswap.c
MAN=	newfs.8

WARNS=	2

MOUNT=	${.CURDIR}/../mount
DISKLABEL=${.CURDIR}/../disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${MOUNT} -I${DISKLABEL}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${MOUNT} ${DISKLABEL} ${.CURDIR}/../../sys/ufs/ffs 

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8 newfs.8 mfs.8

.include <bsd.prog.mk>
