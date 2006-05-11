#	$NetBSD: Makefile,v 1.30 2006/05/11 23:16:29 mrg Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

.include <bsd.own.mk>

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c ffs_bswap.c ffs_appleufs.c
MAN=	newfs.8 mount_mfs.8

DISKLABEL=${NETBSDSRCDIR}/sbin/disklabel
CPPFLAGS+=-DMFS -I${.CURDIR} -I${DISKLABEL}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${DISKLABEL} ${NETBSDSRCDIR}/sys/ufs/ffs 

LINKS=	${BINDIR}/newfs ${BINDIR}/mount_mfs
MLINKS=	mount_mfs.8 mfs.8

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.ffs_appleufs.c+=	-Wno-pointer-sign
.endif
