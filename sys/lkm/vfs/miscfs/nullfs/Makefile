#	$NetBSD: Makefile,v 1.4 1999/02/12 05:41:58 cjs Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs ${.CURDIR}/../..

MKMAN=	no

KMOD=		nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c null_subr.c

.include <bsd.kmod.mk>
