#	$NetBSD: Makefile,v 1.1.24.1 2010/08/17 06:47:42 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/tmpfs

KMOD=	tmpfs
SRCS=	tmpfs_fifoops.c tmpfs_mem.c tmpfs_specops.c tmpfs_subr.c \
	tmpfs_vfsops.c tmpfs_vnops.c

.include <bsd.kmodule.mk>
