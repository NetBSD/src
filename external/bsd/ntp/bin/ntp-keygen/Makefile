#	$NetBSD: Makefile,v 1.5 2013/12/28 03:19:43 christos Exp $

.include <bsd.own.mk>

PROG=	ntp-keygen
MAN=	ntp-keygen.8
BINDIR=	/usr/sbin

SRCS=	ntp-keygen-opts.c	\
	ntp-keygen.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm
DPADD+=		${LIBISC}/libiscntp.a ${LIBOPTS}/libopts.a  ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/util

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
