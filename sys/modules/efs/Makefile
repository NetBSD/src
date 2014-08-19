#	$NetBSD: Makefile,v 1.1.48.1 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/efs

KMOD=	efs
SRCS=	efs_genfs.c efs_subr.c efs_vfsops.c efs_vnops.c

.include <bsd.kmodule.mk>
