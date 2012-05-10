#	$NetBSD: Makefile,v 1.3 2012/05/10 07:51:34 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_rename.c tmpfs_specops.c \
	tmpfs_subr.c tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
