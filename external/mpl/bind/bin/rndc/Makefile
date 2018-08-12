#	$NetBSD: Makefile,v 1.1 2018/08/12 13:02:26 christos Exp $

.include <bsd.own.mk>

PROG=	rndc
SRCS=	rndc.c util.c
MAN=	rndc.8 rndc.conf.5
BINDIR=	/usr/sbin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/rndc
CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \

rndc.o: ${VERSIONFILE}

.PATH: ${DIST}

.include <bsd.prog.mk>
