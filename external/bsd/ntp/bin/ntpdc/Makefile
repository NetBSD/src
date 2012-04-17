#	$NetBSD: Makefile,v 1.4.4.1 2012/04/17 00:03:37 yamt Exp $

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

ntpdc.8:	ntpdc.1
	${_MKTARGET_BUILD}
	${TOOL_SED} -e "s,NTPDC 1,NTPDC 8," $? > $@

CLEANFILES+=ntpdc.8

.include <bsd.prog.mk>
