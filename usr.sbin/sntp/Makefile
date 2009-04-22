# $NetBSD: Makefile,v 1.4 2009/04/22 15:23:08 lukem Exp $

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
