#	$NetBSD: Makefile,v 1.8 1996/08/30 20:22:12 thorpej Exp $

PROG =	rpc.rwalld
SRCS =	rwalld.c
MAN =	rpc.rwalld.8
MLINKS=	rpc.rwalld.8 rwalld.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
