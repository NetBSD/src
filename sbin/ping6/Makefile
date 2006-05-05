# $NetBSD: Makefile,v 1.10 2006/05/05 00:03:22 rpaulo Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC
CPPFLAGS+=-DUSE_RFC3542

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>
