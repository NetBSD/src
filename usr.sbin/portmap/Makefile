#	$NetBSD: Makefile,v 1.10 1999/06/06 03:29:14 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	portmap
MAN=	portmap.8

CPPFLAGS+=-DLIBWRAP
LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

.include <bsd.prog.mk>
