#	$Id: Makefile,v 1.6 1994/01/28 05:13:28 cgd Exp $

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN8 =	rpc.rstatd.0

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
