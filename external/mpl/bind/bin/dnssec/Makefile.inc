#	$NetBSD: Makefile.inc,v 1.3 2024/02/21 22:50:56 christos Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC} ${IDIST}/doc/man

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN?=${BASE}.1
${BASE}.o: ${VERSIONFILE}
