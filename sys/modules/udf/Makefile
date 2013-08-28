#	$NetBSD: Makefile,v 1.2.18.1 2013/08/28 23:59:35 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/udf

KMOD=  	udf
SRCS=	udf_allocation.c udf_osta.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_rmw.c udf_strat_sequential.c udf_subr.c \
	udf_vnops.c udf_vfsops.c udf_rename.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
