#	$NetBSD: Makefile.inc,v 1.2 2020/08/03 17:23:36 christos Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

TOOLS=${IDIST}/bin/tools

.PATH: ${TOOLS} ${IDIST}/doc/man

PROG=${BASE}
SRCS=${BASE}.c
${BASE}.o: ${VERSIONFILE}
.if exists(${IDIST}/doc/man/${BASE}.8in)
BINDIR=/usr/sbin
MAN=${BASE}.8
.elif exists(${IDIST}/doc/man/${BASE}.1in)
BINDIR=/usr/bin
MAN=${BASE}.1
.endif
