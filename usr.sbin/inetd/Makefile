#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.15 1999/06/06 01:53:45 thorpej Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CPPFLAGS+=-DLIBWRAP
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CPPFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

.include <bsd.prog.mk>
