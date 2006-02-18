#	$NetBSD: Makefile,v 1.1.4.2 2006/02/18 15:39:22 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/udf ${.CURDIR}/..

KMOD=	udf

SRCS=	lkminit_vfs.c
SRCS+=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

.include <bsd.kmod.mk>
