#	$NetBSD: Makefile,v 1.1.78.1 2008/06/23 04:31:56 wrstuden Exp $

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
