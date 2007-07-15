#	$NetBSD: Makefile,v 1.1.2.2 2007/07/15 14:52:38 ad Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/efs ${.CURDIR}/..

KMOD=	efs

SRCS=	lkminit_vfs.c
SRCS+=	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmod.mk>
