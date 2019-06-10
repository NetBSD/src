#	$NetBSD: Makefile,v 1.2.28.1 2019/06/10 22:09:23 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_subr.c efs_vfsops.c efs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
