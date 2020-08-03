#	$NetBSD: Makefile.inc,v 1.2 2020/08/03 17:23:35 christos Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC} ${IDIST}/doc/man

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN=${BASE}.8
${BASE}.o: ${VERSIONFILE}
