#	$NetBSD: Makefile,v 1.2.12.1 2012/06/02 11:09:37 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_rename.c tmpfs_specops.c \
	tmpfs_subr.c tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
