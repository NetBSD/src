#	$NetBSD: Makefile,v 1.26 2003/03/22 19:09:39 jdolecek Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c ffs_bswap.c ffs_appleufs.c
MAN=	newfs.8 mount_mfs.8

WARNS=	2

DISKLABEL=${NETBSDSRCDIR}/sbin/disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${DISKLABEL}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${DISKLABEL} ${NETBSDSRCDIR}/sys/ufs/ffs 

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include <bsd.prog.mk>

.if (${MACHINE_ARCH} == "alpha")
#
# XXX	gcc 2.95.3 with -O2 (the default) generates a non-functional newfs
#
CFLAGS+=	-O1
.endif
