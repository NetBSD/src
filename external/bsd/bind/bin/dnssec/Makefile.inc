#	$NetBSD: Makefile.inc,v 1.1.34.1 2017/01/18 08:45:50 skrll Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC}

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN=${BASE}.8
${BASE}.o: ${VERSIONFILE}
