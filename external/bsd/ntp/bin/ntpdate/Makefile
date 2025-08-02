#	$NetBSD: Makefile,v 1.3.60.1 2025/08/02 05:22:08 perseant Exp $

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
