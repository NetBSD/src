#	$Id: Makefile,v 1.3 1995/04/12 00:47:27 jtc Exp $

PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

.include <bsd.prog.mk>
