#	$NetBSD: Makefile,v 1.38.2.1 2012/10/30 18:59:31 yamt Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c ffs_bswap.c ffs_appleufs.c partutil.c
SRCS+=	pathadj.c quota2_subr.c
MAN=	newfs.8 mount_mfs.8

DISKLABEL=${NETBSDSRCDIR}/sbin/disklabel
FSCK=${NETBSDSRCDIR}/sbin/fsck
MOUNT=${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-DMFS -I${.CURDIR} -I${DISKLABEL} -I${FSCK} -I${MOUNT}
CPPFLAGS+=-DGARBAGE

DPADD+= ${LIBUTIL}
LDADD+= -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

.PATH:	${DISKLABEL} ${NETBSDSRCDIR}/sys/ufs/ffs ${NETBSDSRCDIR}/sys/ufs/ufs
.PATH:  ${FSCK} ${MOUNT}

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

COPTS.ffs_appleufs.c+=	-Wno-pointer-sign

.include <bsd.prog.mk>
