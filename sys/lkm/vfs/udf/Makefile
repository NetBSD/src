#	$NetBSD: Makefile,v 1.2 2008/05/14 16:49:48 reinoud Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/udf ${.CURDIR}/..

KMOD=	udf

SRCS=	lkminit_vfs.c
SRCS+=	udf_osta.c udf_subr.c udf_vfsops.c udf_vnops.c \
	udf_readwrite.c udf_strat_bootstrap.c udf_strat_direct.c \
	udf_strat_sequential.c udf_strat_rmw.c udf_allocation.c

CFLAGS+= -DLKM
CFLAGS+= -DPOOL_DIAGNOSTICS
CFLAGS+= -DUDF_READWRITE

.include <bsd.kmod.mk>
