#	$NetBSD: Makefile,v 1.7 2001/12/12 01:50:16 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs $S/miscfs/genfs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c

.include <bsd.kmod.mk>
