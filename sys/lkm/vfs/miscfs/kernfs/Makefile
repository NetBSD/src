#	$NetBSD: Makefile,v 1.9 2001/12/12 01:50:16 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
