#	$NetBSD: Makefile,v 1.2.4.1 2008/10/19 22:18:06 haad Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/udf

LIB=	rumpfs_udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

CFLAGS+=	-Wno-pointer-sign -DDEBUG -DDIAGNOSTIC

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
