#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:13 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_pool.c tmpfs_specops.c tmpfs_subr.c \
	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
