#	$NetBSD: Makefile,v 1.3.2.1 2014/12/25 02:34:21 snj Exp $

.include <bsd.own.mk>

PROG=	ntptime
MAN=	ntptime.8
BINDIR=	/usr/sbin

SRCS=	ntptime.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm
DPADD+=		${LIBISC}/libiscntp.a ${LIBOPTS}/libopts.a  ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/util

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
