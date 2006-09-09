#	$NetBSD: Makefile,v 1.1.22.2 2006/09/09 02:57:59 rpaulo Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/udf ${.CURDIR}/..

KMOD=	udf

SRCS=	lkminit_vfs.c
SRCS+=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

.include <bsd.kmod.mk>
