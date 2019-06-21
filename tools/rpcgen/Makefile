#	$NetBSD: Makefile,v 1.5 2019/06/21 14:56:39 kamil Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}rpcgen
HOST_SRCDIR=	usr.bin/rpcgen

RPCGEN_INC=	${.CURDIR}/../../common/include

HOST_CPPFLAGS+=	-I${RPCGEN_INC}

.include "${.CURDIR}/../Makefile.host"
