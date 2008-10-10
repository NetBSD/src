#	$NetBSD: Makefile,v 1.1.36.3 2008/10/10 22:36:16 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

CFLAGS+=	-Wno-pointer-sign -DDEBUG -DDIAGNOSTIC

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
