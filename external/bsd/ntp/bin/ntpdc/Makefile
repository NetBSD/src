#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:48 kardel Exp $

.include <bsd.own.mk>

PROG=	ntpdc
MAN=
BINDIR=	/usr/sbin

SRCS=	ntpdc.c					\
	ntpdc_ops.c				\
	ntpdc-opts.c


LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm -ledit
DPADD+=		${LIBISC}/libiscntp.a ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL} ${LIBEDIT}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdc

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
