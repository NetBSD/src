#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:48 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_subr.c efs_vfsops.c efs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
