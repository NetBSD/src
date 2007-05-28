# $NetBSD: Makefile,v 1.12 2007/05/28 12:06:23 tls Exp $

USE_FORT?= yes	# setuid
PROG=	ping6
MAN=	ping6.8

BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DINET6
CPPFLAGS+=-DIPSEC

LDADD+=	-lipsec -lm
DPADD+=	${LIBIPSEC} ${LIBM}

.include <bsd.prog.mk>
