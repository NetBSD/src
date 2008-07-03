#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:20 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/unionfs

KMOD=	unionfs
SRCS=	unionfs_vfsops.c unionfs_vnops.c unionfs_subr.c

.include <bsd.kmodule.mk>
