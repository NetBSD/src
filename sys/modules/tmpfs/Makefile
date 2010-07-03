#	$NetBSD: Makefile,v 1.1.26.1 2010/07/03 01:19:59 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_specops.c tmpfs_subr.c \
	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
