#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:18 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/unionfs

KMOD=	unionfs
SRCS=	unionfs_vfsops.c unionfs_vnops.c unionfs_subr.c

.include <bsd.kmodule.mk>
