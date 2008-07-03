#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:20 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_pool.c tmpfs_specops.c tmpfs_subr.c \
	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
