#	$NetBSD: Makefile,v 1.6 1997/10/08 09:07:30 mrg Exp $

PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
