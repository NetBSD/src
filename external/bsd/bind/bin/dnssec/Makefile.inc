#	$NetBSD: Makefile.inc,v 1.1.30.1 2016/11/05 17:47:30 martin Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC}

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN=${BASE}.8
${BASE}.o: ${VERSIONFILE}
