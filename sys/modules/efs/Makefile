#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:58 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
