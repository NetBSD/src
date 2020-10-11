#	$NetBSD: Makefile,v 1.7 2020/10/11 07:07:31 rin Exp $

.include <bsd.own.mk>

PROG=	ntpdc
MAN=	ntpdc.8
BINDIR=	/usr/sbin

SRCS=	ntpdc.c					\
	ntpdc_ops.c				\
	ntpdc-opts.c


LDADD+=		-L${LIBNTP} -lntp -L${LIBISC} -liscntp -lm -ledit -lterminfo
DPADD+=		${LIBISC}/libiscntp.a ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpdc

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/sntp/libopts

.PATH: ${DIST}

CLEANFILES+=${MAN}

.include <bsd.prog.mk>
