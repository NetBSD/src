#	$NetBSD: Makefile,v 1.8.20.1 2002/01/08 00:33:27 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

KMOD=	union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
