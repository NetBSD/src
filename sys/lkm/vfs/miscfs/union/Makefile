#	$NetBSD: Makefile,v 1.7 1999/02/12 05:41:59 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

MKMAN=	no

KMOD=		union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
