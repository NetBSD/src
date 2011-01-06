#	$NetBSD: Makefile.inc,v 1.1.2.1 2011/01/06 21:42:49 riz Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DIST=${IDIST}/bin/confgen

.PATH: ${DIST}/unix ${DIST}

PROG=${BASE}
SRCS=${BASE}.c util.c keygen.c os.c
MAN=${BASE}.8

CPPFLAGS+= -I${DIST}/include \
        -DRNDC_CONFFILE=\"${SYSCONFDIR}/rndc.conf\" \
	-DRNDC_KEYFILE=\"${SYSCONFDIR}/rndc.key\" 

