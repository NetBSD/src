#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:48 kardel Exp $

.include <bsd.own.mk>

PROG=	ntpdate
MAN=
BINDIR=	/usr/sbin

SRCS=	ntpdate.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm 
DPADD+=		${LIBNTP}/libntp.a ${LIBISC}/libiscntp.a ${LIBM}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdate

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
