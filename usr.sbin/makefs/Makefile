#	$NetBSD: Makefile,v 1.8 2002/01/31 22:44:02 tv Exp $
#

PROG=	makefs
SRCS=	makefs.c walk.c \
	ffs.c mkfs.c buf.c \
	getid.c misc.c spec.c pack_dev.c stat_flags.c strsuftoull.c \
	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c ufs_bmap.c 
MAN=	makefs.8

DDSRC=		${.CURDIR}/../../bin/dd
LSSRC=		${.CURDIR}/../../bin/ls
MKNODSRC=	${.CURDIR}/../../sbin/mknod
MTREESRC=	${.CURDIR}/../../usr.sbin/mtree
UFSSRC=		${.CURDIR}/../../sys/ufs

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
