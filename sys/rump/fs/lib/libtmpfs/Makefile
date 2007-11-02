#	$NetBSD: Makefile,v 1.2.14.2 2007/11/02 12:43:42 joerg Exp $
#

.include <bsd.own.mk>

LIB=	tmpfs

.PATH:	${NETBSDSRCDIR}/sys/fs/tmpfs

SRCS=	tmpfs_fifoops.c tmpfs_specops.c tmpfs_vfsops.c tmpfs_pool.c	\
	tmpfs_subr.c tmpfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
