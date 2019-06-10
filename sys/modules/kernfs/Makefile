#	$NetBSD: Makefile,v 1.2.28.1 2019/06/10 22:09:31 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/kernfs

KMOD=	kernfs
SRCS=	kernfs_vfsops.c kernfs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
