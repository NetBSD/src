#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

MKMAN=	no

KMOD=	portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
