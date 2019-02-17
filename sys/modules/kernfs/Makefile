#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:53 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/kernfs

KMOD=	kernfs
SRCS=	kernfs_vfsops.c kernfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
