#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:16 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
