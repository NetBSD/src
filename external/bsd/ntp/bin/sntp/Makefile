# $NetBSD: Makefile,v 1.2 2010/12/04 23:08:33 christos Exp $

USE_FORT?= yes	# network client/server

PROG=	sntp
MAN=	sntp.1
SRCS=	crypto.c kod_management.c log.c main.c networking.c sntp-opts.c utilities.c
BINDIR=	/usr/sbin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/sntp

CPPFLAGS+=-I${IDIST}/sntp -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.own.mk>

CPPFLAGS+= -DLOCKNAME=\"/var/run/sntp.pid\"
CPPFLAGS+= -DSAVENAME=\"/var/db/sntp.state\"
LDADD+=	-lm -L${LIBISC} -liscntp
DPADD+=	${LIBM}

.include <bsd.prog.mk>
