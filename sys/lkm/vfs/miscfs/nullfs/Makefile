#	$NetBSD: Makefile,v 1.6.14.1 2002/01/08 00:33:24 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs $S/miscfs/genfs ${.CURDIR}/../..

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c

.include <bsd.kmod.mk>
