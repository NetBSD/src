#	$NetBSD: Makefile,v 1.13.26.1 2020/04/13 08:05:46 martin Exp $

PROG=	rpcgen
SRCS=	rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c rpc_scan.c \
	rpc_svcout.c rpc_util.c rpc_sample.c rpc_tblout.c

COPTS.rpc_cout.c+=	${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
