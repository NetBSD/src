#	$NetBSD: Makefile,v 1.2 2010/08/28 15:42:46 kardel Exp $

.include <bsd.own.mk>

PROG=	ntpdate
MAN=	ntpdate.8
BINDIR=	/usr/sbin

SRCS=	ntpdate.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm 
DPADD+=		${LIBNTP}/libntp.a ${LIBISC}/libiscntp.a ${LIBM}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdate

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
