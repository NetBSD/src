#	$NetBSD: Makefile,v 1.6 1998/03/28 23:58:45 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

NOMAN=	yep

KMOD=		union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
