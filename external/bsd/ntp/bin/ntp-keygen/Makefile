#	$NetBSD: Makefile,v 1.3.4.1 2012/04/17 00:03:37 yamt Exp $

NOGCCERROR=yes

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

ntp-keygen.8:	ntp-keygen.1
	${_MKTARGET_BUILD}
	${TOOL_SED} -e "s,NTP-KEYGEN 1,NTP-KEYGEN 8," $? > $@

CLEANFILES+=ntp-keygen.8

.include <bsd.prog.mk>
