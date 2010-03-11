#	$NetBSD: Makefile,v 1.1.34.3 2010/03/11 15:04:36 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
