#	$NetBSD: Makefile,v 1.2.6.1 2012/04/17 00:03:37 yamt Exp $

NOGCCERROR=yes

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
