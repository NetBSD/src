#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.19 2001/11/19 03:18:28 itojun Exp $

PROG=	inetd
SRCS=	inetd.c
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CPPFLAGS+=-DLIBWRAP -DINET6
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CPPFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

CPPFLAGS+=-DIPSEC
SRCS+=	ipsec.c
LDADD+= -lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>
