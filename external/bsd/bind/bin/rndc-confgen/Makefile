#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:49:12 jym Exp $

.include <bsd.own.mk>

PROG=	rndc-confgen
SRCS=	rndc-confgen.c util.c os.c
MAN=	rndc-confgen.8
BINDIR=	/usr/sbin

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/bin/rndc
CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" \


.PATH: ${DIST}/unix ${DIST}

.include <bsd.prog.mk>
