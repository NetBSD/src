#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:48 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

NOMAN=	yep

KMOD=		portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
