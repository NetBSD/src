#	$NetBSD: Makefile,v 1.2 2014/08/07 08:24:23 hannken Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
