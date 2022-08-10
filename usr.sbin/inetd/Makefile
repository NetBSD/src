#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.32 2022/08/10 08:38:43 christos Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	inetd
SRCS=	inetd.c parse.c parse_v2.c ratelimit.c
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5
WARNS=	6
#LINTFLAGS+=	-T

# Enables debug printouts when in debug mode
CPPFLAGS+=-DDEBUG_ENABLE

CPPFLAGS+=-DLIBWRAP
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CPPFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap -lblocklist -lutil
DPADD+= ${LIBWRAP} ${LIBBLOCKLIST} ${LIBUTIL}

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

CPPFLAGS+=-DIPSEC
SRCS+=	ipsec.c
LDADD+= -lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>
