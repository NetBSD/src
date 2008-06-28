#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:35 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
