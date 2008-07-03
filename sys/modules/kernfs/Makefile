#	$NetBSD: Makefile,v 1.1.4.2 2008/07/03 18:38:19 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/kernfs

KMOD=	kernfs
SRCS=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmodule.mk>
