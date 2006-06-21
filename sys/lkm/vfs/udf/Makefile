#	$NetBSD: Makefile,v 1.1.16.2 2006/06/21 15:10:24 yamt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/udf ${.CURDIR}/..

KMOD=	udf

SRCS=	lkminit_vfs.c
SRCS+=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c

.include <bsd.kmod.mk>
