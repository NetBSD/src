#	$NetBSD: Makefile,v 1.1 2006/02/02 15:19:16 reinoud Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/udf ${.CURDIR}/..

KMOD=	udf

SRCS=	lkminit_vfs.c
SRCS+=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

.include <bsd.kmod.mk>
