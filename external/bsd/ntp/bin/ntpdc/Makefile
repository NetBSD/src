#	$NetBSD: Makefile,v 1.6.6.2 2015/01/07 04:45:13 msaitoh Exp $

.include <bsd.own.mk>

PROG=	ntpdc
MAN=	ntpdc.8
BINDIR=	/usr/sbin

SRCS=	ntpdc.c					\
	ntpdc_ops.c				\
	ntpdc-opts.c


LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm -ledit -ltermcap
DPADD+=		${LIBISC}/libiscntp.a ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL} ${LIBEDIT} ${LIBTERMCAP}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdc

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
