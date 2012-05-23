#	$NetBSD: Makefile,v 1.4.8.1 2012/05/23 10:08:16 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../fs/tmpfs

LIB=	rumpfs_tmpfs

SRCS=	tmpfs_fifoops.c tmpfs_specops.c tmpfs_vfsops.c tmpfs_mem.c \
	tmpfs_rename.c tmpfs_subr.c tmpfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
