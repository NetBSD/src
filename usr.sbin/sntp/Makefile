# $NetBSD: Makefile,v 1.3.20.1 2009/05/13 19:20:40 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wsign-compare -Wcast-qual

USE_FORT?= yes	# network client/server

PROG=	sntp
SRCS=	internet.c main.c socket.c timing.c unix.c

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist/ntp

CPPFLAGS+= -DLOCKNAME=\"/var/run/sntp.pid\"
CPPFLAGS+= -DSAVENAME=\"/var/db/sntp.state\"
LDADD+=	-lm
DPADD+=	${LIBM}

.PATH:	${DIST}/sntp

.include <bsd.prog.mk>
