#	$NetBSD: Makefile,v 1.1.18.1 2009/05/13 19:19:04 jym Exp $
#	@(#)Makefile	8.2 (Berkeley) 3/27/94

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	newfs_ext2fs
SRCS=	newfs_ext2fs.c mke2fs.c ext2fs_bswap.c partutil.c
MAN=	newfs_ext2fs.8

FSCK=${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${.CURDIR} -I${FSCK}
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.PATH:	${NETBSDSRCDIR}/sys/ufs/ext2fs ${FSCK}

.include <bsd.prog.mk>
