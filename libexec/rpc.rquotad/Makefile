#	$NetBSD: Makefile,v 1.8 2011/03/24 17:05:43 bouyer Exp $

.include <bsd.own.mk>
PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8
MLINKS = rpc.rquotad.8 rquotad.8

DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBPROP}
LDADD=	-lquota -lrpcsvc -lprop

.include <bsd.prog.mk>
