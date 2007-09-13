#	$NetBSD: Makefile,v 1.2 2007/09/13 21:46:54 rumble Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/efs ${.CURDIR}/..

KMOD=	efs

SRCS=	lkminit_vfs.c
SRCS+=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmod.mk>
