#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 11:41:08 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/udf

KMOD=  	udf
SRCS=	udf_allocation.c udf_osta.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_rmw.c udf_strat_sequential.c udf_subr.c \
	udf_vnops.c udf_vfsops.c udf_rename.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
