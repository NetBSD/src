#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:09 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
