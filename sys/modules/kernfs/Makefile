#	$NetBSD: Makefile,v 1.1.2.2 2008/06/29 09:33:17 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/kernfs

KMOD=	kernfs
SRCS=	kernfs_vfsops.c kernfs_vnops.c kernfs_subr.c

.include <bsd.kmodule.mk>
