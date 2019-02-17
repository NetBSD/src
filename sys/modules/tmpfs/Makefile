#	$NetBSD: Makefile,v 1.4 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_rename.c tmpfs_specops.c \
	tmpfs_subr.c tmpfs_vfsops.c tmpfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
