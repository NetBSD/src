#	$NetBSD: Makefile,v 1.8 2000/01/31 14:20:13 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rlogind
MAN=	rlogind.8
DPADD=	${LIBUTIL}
LDADD=	-lutil

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>
