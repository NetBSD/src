#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:44 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
