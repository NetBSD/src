#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/kernfs

KMOD=	kernfs
SRCS=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmodule.mk>
