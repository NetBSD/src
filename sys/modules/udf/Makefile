#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:31 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/udf

KMOD=  	udf
SRCS=	udf_allocation.c udf_osta.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_rmw.c udf_strat_sequential.c udf_subr.c \
	udf_vnops.c udf_vfsops.c

CWARNFLAGS.clang=	-Wno-self-assign

.include <bsd.kmodule.mk>
