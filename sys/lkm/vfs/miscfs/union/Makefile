#	$NetBSD: Makefile,v 1.9 2001/12/12 01:50:17 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/union ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
