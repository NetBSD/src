#	$NetBSD: Makefile,v 1.6 1999/07/08 01:51:57 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs $S/miscfs/genfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c

.include <bsd.kmod.mk>
