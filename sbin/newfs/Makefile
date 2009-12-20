#	$NetBSD: Makefile,v 1.35 2009/12/20 15:21:13 dsl Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c ffs_bswap.c ffs_appleufs.c partutil.c
SRCS+=	pathadj.c
MAN=	newfs.8 mount_mfs.8

DISKLABEL=${NETBSDSRCDIR}/sbin/disklabel
FSCK=${NETBSDSRCDIR}/sbin/fsck
MOUNT=${NETBSDSRCDIR}/sbin/mount
CPPFLAGS+=-DMFS -I${.CURDIR} -I${DISKLABEL} -I${FSCK} -I${MOUNT}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

.PATH:	${DISKLABEL} ${NETBSDSRCDIR}/sys/ufs/ffs ${FSCK} ${MOUNT}

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.ffs_appleufs.c+=	-Wno-pointer-sign
.endif
