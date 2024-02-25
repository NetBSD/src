#	$NetBSD: Makefile.inc,v 1.2.6.1 2024/02/25 15:42:53 martin Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC} ${IDIST}/doc/man

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN?=${BASE}.1
${BASE}.o: ${VERSIONFILE}
