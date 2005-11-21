#	$NetBSD: Makefile.inc,v 1.1.2.2 2005/11/21 20:36:31 tron Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC}

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN=${BASE}.8
