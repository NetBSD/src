#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:46 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/nullfs ${.CURDIR}/../..

NOMAN=	yep

KMOD=		nullfs

SRCS=	lkminit_vfs.c
SRCS+=	null_vfsops.c null_vnops.c null_subr.c

.include <bsd.kmod.mk>
