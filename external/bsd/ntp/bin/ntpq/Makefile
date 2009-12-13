#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:48 kardel Exp $

# NOGCCERROR=yes

.include <bsd.own.mk>

PROG=	ntpq
MAN=
BINDIR=	/usr/sbin

SRCS=	ntpq.c					\
	ntpq-subs.c				\
	ntpq-opts.c

LDADD+=		-L${LIBNTP} -lntp -L${LIBOPTS} -lopts -L${LIBISC} -liscntp -ledit -lm 
#DPADD+=		${LIBISC}/libiscntp.a ${LIBOPTS}/libopts.a  ${LIBNTP}/libntp.a ${LIBM} ${LIBUTIL}

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/ntpq

CPPFLAGS+=-I${IDIST}/include -I${IDIST}/libopts

.PATH: ${DIST}

.include <bsd.prog.mk>
