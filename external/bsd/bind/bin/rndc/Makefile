#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:49:11 jym Exp $

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


.PATH: ${DIST}

.include <bsd.prog.mk>
