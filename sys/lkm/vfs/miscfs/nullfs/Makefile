#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c null_subr.c

.include <bsd.kmod.mk>
