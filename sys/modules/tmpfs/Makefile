#	$NetBSD: Makefile,v 1.3.40.1 2019/06/10 22:09:40 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_rename.c tmpfs_specops.c \
	tmpfs_subr.c tmpfs_vfsops.c tmpfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
