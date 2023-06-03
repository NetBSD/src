#	$NetBSD: Makefile,v 1.15 2023/06/03 09:09:18 lukem Exp $

PROG=	rpcgen
SRCS=	rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c rpc_scan.c \
	rpc_svcout.c rpc_util.c rpc_sample.c rpc_tblout.c

COPTS.rpc_cout.c+=	${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.prog.mk>
