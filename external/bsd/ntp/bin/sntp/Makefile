# $NetBSD: Makefile,v 1.2.6.1 2012/04/17 00:03:37 yamt Exp $

USE_FORT?= yes	# network client/server

PROG=	sntp
MAN=	sntp.1
SRCS=	crypto.c kod_management.c log.c main.c networking.c sntp-opts.c utilities.c sntp.c
BINDIR=	/usr/sbin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/sntp

CPPFLAGS+=-I${IDIST}/sntp -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.own.mk>

CPPFLAGS+= -DLOCKNAME=\"/var/run/sntp.pid\"
CPPFLAGS+= -DSAVENAME=\"/var/db/sntp.state\"
LDADD+=	-lm -L${LIBISC} -liscntp
DPADD+=	${LIBM}

.include <bsd.prog.mk>
