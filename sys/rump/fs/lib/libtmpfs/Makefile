#	$NetBSD: Makefile,v 1.2.2.2 2007/08/15 13:50:29 skrll Exp $
#

.include <bsd.own.mk>

LIB=	tmpfs

.PATH:	${NETBSDSRCDIR}/sys/fs/tmpfs

SRCS=	tmpfs_fifoops.c tmpfs_specops.c tmpfs_vfsops.c tmpfs_pool.c	\
	tmpfs_subr.c tmpfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
