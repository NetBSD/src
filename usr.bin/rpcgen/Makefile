#	$Id: Makefile,v 1.2 1993/08/02 17:55:35 mycroft Exp $

PROG=	rpcgen
SRCS= rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c rpc_scan.c \
      rpc_svcout.c rpc_util.c 
LDADD= -lrpc
DPADD = ${LIBRPC}
MAN1 = rpcgen.0

.include <bsd.prog.mk>
