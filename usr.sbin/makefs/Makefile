#	$NetBSD: Makefile,v 1.1.1.1 2001/10/26 06:56:13 lukem Exp $
#

PROG=	makefs
SRCS=	makefs.c walk.c \
	ffs.c mkfs.c buf.c \
	misc.c spec.c pack_dev.c stat_flags.c \
	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c ufs_bmap.c 
LDADD+=-lutil

LSSRC=		${.CURDIR}/../../bin/ls
MKNODSRC=	${.CURDIR}/../../sbin/mknod
MTREESRC=	${.CURDIR}/../../usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${LSSRC} -I${MKNODSRC} -I${MTREESRC}
.PATH:		${.CURDIR}/ffs ${.CURDIR}/../../sys/ufs/ffs \
		${LSSRC} ${MKNODSRC} ${MTREESRC}

WARNS?=	2

.include <bsd.prog.mk>
