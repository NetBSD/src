#	$NetBSD: Makefile,v 1.4 1999/02/12 05:41:58 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

MKMAN=	no

KMOD=		fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
