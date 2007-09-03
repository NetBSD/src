# $NetBSD: Makefile,v 1.2.18.1 2007/09/03 07:05:44 wrstuden Exp $

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
