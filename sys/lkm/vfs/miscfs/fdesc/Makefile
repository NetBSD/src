#	$NetBSD: Makefile,v 1.5.20.1 2002/01/08 00:33:23 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

KMOD=	fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
