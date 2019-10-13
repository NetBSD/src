#	$NetBSD: Makefile,v 1.14 2019/10/13 07:28:16 mrg Exp $

PROG=	rpcgen
SRCS=	rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c rpc_scan.c \
	rpc_svcout.c rpc_util.c rpc_sample.c rpc_tblout.c

COPTS.rpc_cout.c+=	${GCC_NO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
