#	$NetBSD: Makefile,v 1.4 2019/02/17 04:05:59 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/udf

KMOD=  	udf
SRCS=	udf_allocation.c udf_osta.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_rmw.c udf_strat_sequential.c udf_subr.c \
	udf_vnops.c udf_vfsops.c udf_rename.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
