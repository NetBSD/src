#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:40 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

NOMAN=	yep

KMOD=		fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
