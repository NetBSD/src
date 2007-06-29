#	$NetBSD: Makefile,v 1.1 2007/06/29 23:30:31 rumble Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/efs ${.CURDIR}/..

KMOD=	efs

SRCS=	lkminit_vfs.c
SRCS+=	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmod.mk>
