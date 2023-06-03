#	$NetBSD: Makefile,v 1.8 2023/06/03 08:52:58 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/udf

KMOD=	udf
SRCS=	udf_allocation.c udf_osta.c udf_readwrite.c udf_strat_bootstrap.c \
	udf_strat_direct.c udf_strat_rmw.c udf_strat_sequential.c udf_subr.c \
	udf_vnops.c udf_vfsops.c udf_rename.c

WARNS=	3

CWARNFLAGS.clang=	-Wno-self-assign

COPTS.udf_allocation.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}
COPTS.udf_subr.c+=		${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.kmodule.mk>
