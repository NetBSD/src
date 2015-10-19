#	$NetBSD: Makefile,v 1.9 2015/10/19 16:16:36 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf
COMMENT=UDF

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c \
	udf_rename.c

CWARNFLAGS.clang+=	-Wno-self-assign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
