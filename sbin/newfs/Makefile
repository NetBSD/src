#	$NetBSD: Makefile,v 1.25 2003/03/22 15:34:44 tron Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c ffs_bswap.c ffs_appleufs.c
MAN=	newfs.8 mount_mfs.8

WARNS=	2

MOUNT=	${NETBSDSRCDIR}/sbin/mount
DISKLABEL=${NETBSDSRCDIR}/sbin/disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${MOUNT} -I${DISKLABEL}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${MOUNT} ${DISKLABEL} ${NETBSDSRCDIR}/sys/ufs/ffs 

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include <bsd.prog.mk>

.if (${MACHINE_ARCH} == "alpha")
#
# XXX	gcc 2.95.3 with -O2 (the default) generates a non-functional newfs
#
CFLAGS+=	-O1
.endif
