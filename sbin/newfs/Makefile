#	$NetBSD: Makefile,v 1.16 1998/03/18 17:10:15 bouyer Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

PROG=	newfs
SRCS=	dkcksum.c getmntopts.c newfs.c mkfs.c ffs_bswap.c
MAN=	newfs.8

MOUNT=	${.CURDIR}/../mount
DISKLABEL=${.CURDIR}/../disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${MOUNT} -I${DISKLABEL}
.PATH:	${MOUNT} ${DISKLABEL} ${.CURDIR}/../../sys/ufs/ffs 

DPADD=	${LIBUTIL}
LDADD=	-lutil

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	newfs.8 mount_mfs.8 newfs.8 mfs.8

.include <bsd.prog.mk>
