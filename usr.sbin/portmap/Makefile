#	$NetBSD: Makefile,v 1.7 1997/10/17 05:37:32 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	portmap
MAN=	portmap.8

DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
