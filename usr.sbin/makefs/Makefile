#	$NetBSD: Makefile,v 1.9 2002/09/18 03:54:33 lukem Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=	makefs.c walk.c \
	ffs.c mkfs.c buf.c \
	getid.c misc.c spec.c pack_dev.c stat_flags.c strsuftoull.c \
	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c ufs_bmap.c 
MAN=	makefs.8

DDSRC=		${NETBSDSRCDIR}/bin/dd
LSSRC=		${NETBSDSRCDIR}/bin/ls
MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree
UFSSRC=		${NETBSDSRCDIR}/sys/ufs

CPPFLAGS+=	-I${.CURDIR} \
		-I${LSSRC} -I${MKNODSRC} -I${MTREESRC} -I${DDSRC} 
.PATH:		${.CURDIR}/ffs ${UFSSRC}/ffs \
		${LSSRC} ${MKNODSRC} ${MTREESRC} ${DDSRC} 

WARNS?=	2

.ifndef HOSTPROG
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
.endif
