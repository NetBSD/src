#	$NetBSD: Makefile,v 1.4 1996/08/30 20:15:06 thorpej Exp $

PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
