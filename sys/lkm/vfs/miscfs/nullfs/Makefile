#	$NetBSD: Makefile,v 1.8 2001/12/12 12:06:48 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs $S/miscfs/genfs ${.CURDIR}/../..

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c

.include <bsd.kmod.mk>
