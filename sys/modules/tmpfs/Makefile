#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_pool.c tmpfs_specops.c tmpfs_subr.c \
	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
