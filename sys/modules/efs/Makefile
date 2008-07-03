#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:19 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_ihash.c	efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
