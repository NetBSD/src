#	$NetBSD: Makefile,v 1.6.16.1 2002/01/10 20:01:21 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs $S/miscfs/genfs ${.CURDIR}/../..

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c

.include <bsd.kmod.mk>
