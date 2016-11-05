#	$NetBSD: Makefile.inc,v 1.1.24.1 2016/11/05 17:47:30 martin Exp $

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

TOOLS=${IDIST}/bin/tools

.PATH: ${TOOLS}

PROG=${BASE}
SRCS=${BASE}.c
${BASE}.o: ${VERSIONFILE}
.if exists(${TOOLS}/${BASE}.8)
BINDIR=/usr/sbin
MAN=${BASE}.8
.elif exists(${TOOLS}/${BASE}.1)
BINDIR=/usr/bin
MAN=${BASE}.1
.endif
