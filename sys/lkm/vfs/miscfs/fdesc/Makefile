#	$NetBSD: Makefile,v 1.5.22.1 2002/01/10 20:01:20 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

KMOD=	fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
