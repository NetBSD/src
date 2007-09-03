#	$NetBSD: Makefile,v 1.1.1.1.20.2 2007/09/03 14:41:40 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/hfs ${.CURDIR}/..

KMOD=	hfs

SRCS=	lkminit_vfs.c
SRCS+=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c
SRCS+=	unicode.c

.include <bsd.kmod.mk>
