#	$NetBSD: Makefile,v 1.6 2002/01/07 16:27:22 lukem Exp $
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
UFSSRC=		${.CURDIR}/../../sys/ufs

CPPFLAGS+=	-I${.CURDIR} \
		-I${LSSRC} -I${MKNODSRC} -I${MTREESRC} -I${DDSRC} 
.PATH:		${.CURDIR}/ffs ${UFSSRC}/ffs \
		${LSSRC} ${MKNODSRC} ${MTREESRC} ${DDSRC} 

WARNS?=	2

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
