#	$NetBSD: Makefile,v 1.22 2002/01/08 20:54:49 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c ffs_bswap.c
MAN=	newfs.8 mount_mfs.8

WARNS=	2

MOUNT=	${.CURDIR}/../mount
DISKLABEL=${.CURDIR}/../disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${MOUNT} -I${DISKLABEL}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${MOUNT} ${DISKLABEL} ${.CURDIR}/../../sys/ufs/ffs 

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include <bsd.prog.mk>

.if (${MACHINE_ARCH} == "alpha")
#
# XXX	gcc 2.95.3 with -O2 (the default) generates a non-functional newfs
#
CFLAGS+=	-O1
.endif
