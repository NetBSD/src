#	$NetBSD: Makefile,v 1.6 2001/12/12 01:50:17 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/portal ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	portal

SRCS=	lkminit_vfs.c
SRCS+=	portal_vfsops.c portal_vnops.c

.include <bsd.kmod.mk>
