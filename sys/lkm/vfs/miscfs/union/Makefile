#	$NetBSD: Makefile,v 1.8 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

MKMAN=	no

KMOD=	union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
