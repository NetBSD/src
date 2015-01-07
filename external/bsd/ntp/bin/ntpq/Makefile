#	$NetBSD: Makefile,v 1.7.6.2 2015/01/07 04:45:13 msaitoh Exp $

.include <bsd.own.mk>

PROG=	ntpq
MAN=	ntpq.8
BINDIR=	/usr/sbin

SRCS=	ntpq.c					\
	ntpq-subs.c				\
	ntpq-opts.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBOPTS} -lopts -L${LIBISC} -liscntp -ledit -ltermcap -lm 
DPADD+=		${LIBISC}/libiscntp.a ${LIBOPTS}/libopts.a  ${LIBNTP}/libntp.a ${LIBEDIT} ${LIBTERMCAP} ${LIBM}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpq

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
