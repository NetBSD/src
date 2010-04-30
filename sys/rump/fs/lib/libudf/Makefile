#	$NetBSD: Makefile,v 1.5.6.1 2010/04/30 14:44:28 uebayasi Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
