#	$NetBSD: Makefile,v 1.8 2021/04/13 04:58:59 mrg Exp $

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

COPTS.ntpdc.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-array-bounds :}

.include <bsd.prog.mk>
