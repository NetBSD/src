#	$NetBSD: Makefile,v 1.13 2002/11/30 21:56:34 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	rshd
MAN=	rshd.8

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

CPPFLAGS+=-DLOGIN_CAP

CPPFLAGS+=-DINET6

.include <bsd.prog.mk>
