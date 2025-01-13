#	$NetBSD: Makefile,v 1.4 2025/01/13 07:53:47 skrll Exp $

.include <bsd.own.mk>

PROG=	ntpdate
MAN=	ntpdate.8
BINDIR=	/usr/sbin

SRCS=	ntpdate.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm
DPADD+=		${LIBNTP}/libntp.a ${LIBISC}/libiscntp.a ${LIBM}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdate

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
