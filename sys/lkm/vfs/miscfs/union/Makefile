#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:53 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

NOMAN=	yep

KMOD=		union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
