#	$NetBSD: Makefile,v 1.1.64.1 2014/08/10 06:56:10 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
