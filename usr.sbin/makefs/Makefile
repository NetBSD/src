#	$NetBSD: Makefile,v 1.4 2001/11/25 11:22:09 lukem Exp $
#

PROG=	makefs
SRCS=	makefs.c walk.c \
	ffs.c mkfs.c buf.c \
	misc.c spec.c pack_dev.c stat_flags.c strsuftoull.c \
	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c ufs_bmap.c 
MAN=	makefs.8
DPADD+=	${LIBUTIL}
LDADD+=	-lutil

DDSRC=		${.CURDIR}/../../bin/dd
LSSRC=		${.CURDIR}/../../bin/ls
MKNODSRC=	${.CURDIR}/../../sbin/mknod
MTREESRC=	${.CURDIR}/../../usr.sbin/mtree
SYSSRC=		${.CURDIR}/../../sys

CPPFLAGS+=	-I${.CURDIR} -I${SYSSRC} \
		-I${LSSRC} -I${MKNODSRC} -I${MTREESRC} -I${DDSRC} 
.PATH:		${.CURDIR}/ffs ${SYSSRC}/ufs/ffs \
		${LSSRC} ${MKNODSRC} ${MTREESRC} ${DDSRC} 

WARNS?=	2

.include <bsd.prog.mk>
