#	$NetBSD: Makefile,v 1.5.20.1 2002/01/08 00:33:25 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

KMOD=	portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
