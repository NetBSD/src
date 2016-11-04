#	$NetBSD: Makefile.inc,v 1.1.32.1 2016/11/04 14:42:41 pgoyette Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

BINDIR=/usr/sbin
DNSSEC=${IDIST}/bin/dnssec

.PATH: ${DNSSEC}

PROG=${BASE}
SRCS=${BASE}.c dnssectool.c
MAN=${BASE}.8
${BASE}.o: ${VERSIONFILE}
