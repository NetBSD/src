#	$NetBSD: Makefile,v 1.4 1999/02/12 05:41:59 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

MKMAN=	no

KMOD=		portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
