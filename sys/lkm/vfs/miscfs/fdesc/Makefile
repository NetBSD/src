#	$NetBSD: Makefile,v 1.6 2001/12/12 01:50:16 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/fdesc ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	fdesc

SRCS=	lkminit_vfs.c
SRCS+=	fdesc_vfsops.c fdesc_vnops.c

.include <bsd.kmod.mk>
