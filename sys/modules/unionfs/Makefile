#	$NetBSD: Makefile,v 1.1.8.2 2008/09/18 04:36:59 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/unionfs

KMOD=	unionfs
SRCS=	unionfs_vfsops.c unionfs_vnops.c unionfs_subr.c

.include <bsd.kmodule.mk>
