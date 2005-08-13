#	$NetBSD: Makefile,v 1.15 2005/08/13 01:53:01 fvdl Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=	makefs.c walk.c \
	ffs.c mkfs.c buf.c \
	getid.c misc.c spec.c pack_dev.c stat_flags.c \
	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c \
	ufs_bmap.c \
	cd9660.c cd9660_strings.c cd9660_debug.c cd9660_eltorito.c \
	cd9660_write.c cd9660_conversion.c iso9660_rrip.c
MAN=	makefs.8

LSSRC=		${NETBSDSRCDIR}/bin/ls
MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree
UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CD9660SRC=	${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=	-I${.CURDIR} \
		-I${LSSRC} -I${MKNODSRC} -I${MTREESRC} -I${CD9660SRC}
.PATH:		${.CURDIR}/ffs ${UFSSRC}/ffs \
		${.CURDIR}/cd9660 ${CD9660SRC} \
		${LSSRC} ${MKNODSRC} ${MTREESRC}

WARNS?=	3

.include <bsd.prog.mk>
