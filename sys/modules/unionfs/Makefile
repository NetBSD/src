#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:14 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/unionfs

KMOD=	unionfs
SRCS=	unionfs_vfsops.c unionfs_vnops.c unionfs_subr.c

.include <bsd.kmodule.mk>
