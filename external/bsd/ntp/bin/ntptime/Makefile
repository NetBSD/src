#	$NetBSD: Makefile,v 1.2.6.2 2014/05/22 15:49:55 yamt Exp $

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
