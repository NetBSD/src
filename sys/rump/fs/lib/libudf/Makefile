#	$NetBSD: Makefile,v 1.2.2.1 2008/07/31 04:51:04 simonb Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

CFLAGS+=	-Wno-pointer-sign -DUDF_READWRITE -DDEBUG -DDIAGNOSTIC

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
