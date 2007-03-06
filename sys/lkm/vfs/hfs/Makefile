#	$NetBSD: Makefile,v 1.1.1.1 2007/03/06 11:27:15 dillo Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/hfs ${.CURDIR}/..

KMOD=	hfs

SRCS=	lkminit_vfs.c
SRCS+=	hfs_nhash.c hfs_subr.c hfs_vfsops.c hfs_vnops.c libhfs.c
SRCS+=	unicode.c

.include <bsd.kmod.mk>
