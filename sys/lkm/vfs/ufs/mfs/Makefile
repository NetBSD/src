#	$NetBSD: Makefile,v 1.6 2001/12/12 01:50:19 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/ufs/mfs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	mfs

SRCS=	lkminit_vfs.c
SRCS+=	mfs_vfsops.c mfs_vnops.c

.include <bsd.kmod.mk>
