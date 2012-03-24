#	$NetBSD: Makefile,v 1.4 2012/03/24 08:22:24 joerg Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

WARNS?=	4

CWARNFLAGS+=	-Wno-sign-compare

.include <bsd.own.mk>

PROG=	newfs_ext2fs
SRCS=	newfs_ext2fs.c mke2fs.c ext2fs_bswap.c partutil.c
MAN=	newfs_ext2fs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${.CURDIR} -I${FSCK}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

LDADD+=-lprop
DPADD+=${LIBPROP}

.PATH:	${NETBSDSRCDIR}/sys/ufs/ext2fs ${FSCK}

.include <bsd.prog.mk>
