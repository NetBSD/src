#	$NetBSD: Makefile,v 1.2 2010/08/28 15:42:46 kardel Exp $

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

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

ntp-keygen.8:	ntp-keygen.1
	@cp $? $@

.include <bsd.prog.mk>
