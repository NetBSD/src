#	$Id: Makefile,v 1.8 1995/04/12 00:47:43 jtc Exp $

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
