#	$NetBSD: Makefile,v 1.5.22.1 2002/01/10 20:01:22 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

KMOD=	portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
