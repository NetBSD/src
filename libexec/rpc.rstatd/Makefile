#	$NetBSD: Makefile,v 1.9 1996/08/30 20:17:40 thorpej Exp $

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
