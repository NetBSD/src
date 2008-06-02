#	$NetBSD: Makefile,v 1.1.30.1 2008/06/02 13:24:31 mjf Exp $
#

.include <bsd.own.mk>

LIB=	udf

.PATH:  ${NETBSDSRCDIR}/sys/fs/udf

SRCS=	udf_osta.c udf_vfsops.c udf_vnops.c udf_subr.c \
	udf_allocation.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_sequential.c udf_strat_rmw.c

CFLAGS+=	-Wno-pointer-sign -DUDF_READWRITE -DDEBUG -DDIAGNOSTIC

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
