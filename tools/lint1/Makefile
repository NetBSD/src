#	$NetBSD: Makefile,v 1.3 2001/10/25 02:23:21 thorpej Exp $

XLINT_TARGET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${XLINT_TARGET_PREFIX}lint1
HOST_SRCDIR=	usr.bin/xlint/lint1
HOST_BINDIR=	${TOOLDIR}/libexec

.include "${.CURDIR}/../Makefile.host"

.include "${_CURDIR}/../lint/Makefile.inc"

${TIMESTAMP}: ${SRCS}
