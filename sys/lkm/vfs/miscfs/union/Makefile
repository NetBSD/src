#	$NetBSD: Makefile,v 1.11 2003/03/16 08:26:49 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/union ${.CURDIR}/../..

KMOD=	union

SRCS=	lkminit_vfs.c
SRCS+=	union_vfsops.c union_vnops.c union_subr.c

.include <bsd.kmod.mk>
