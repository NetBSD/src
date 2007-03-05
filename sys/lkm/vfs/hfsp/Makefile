#	$NetBSD: Makefile,v 1.1.1.1 2007/03/05 23:09:16 dillo Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/hfsp ${.CURDIR}/..

KMOD=	hfsp

SRCS=	lkminit_vfs.c
SRCS+=	hfsp_nhash.c hfsp_subr.c hfsp_vfsops.c hfsp_vnops.c libhfsp.c
SRCS+=	unicode.c

.include <bsd.kmod.mk>
