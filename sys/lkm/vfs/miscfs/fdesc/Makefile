#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

MKMAN=	no

KMOD=	fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
