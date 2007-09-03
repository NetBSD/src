#	$NetBSD: Makefile,v 1.1.12.2 2007/09/03 14:41:39 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/efs ${.CURDIR}/..

KMOD=	efs

SRCS=	lkminit_vfs.c
SRCS+=	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmod.mk>
