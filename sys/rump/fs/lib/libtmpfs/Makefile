#	$NetBSD: Makefile,v 1.4 2010/06/22 18:32:08 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../fs/tmpfs

LIB=	rumpfs_tmpfs

SRCS=	tmpfs_fifoops.c tmpfs_specops.c tmpfs_vfsops.c tmpfs_mem.c \
	tmpfs_subr.c tmpfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
