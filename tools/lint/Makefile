#	$NetBSD: Makefile,v 1.3 2001/10/25 02:23:21 thorpej Exp $

XLINT_TARGET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${XLINT_TARGET_PREFIX}lint
HOST_SRCDIR=	usr.bin/xlint/xlint
HOST_CPPFLAGS=	-DPATH_LIBEXEC=\"${TOOLDIR}/libexec\"

.include "${.CURDIR}/../Makefile.host"

HOST_CPPFLAGS+=-DTARGET_PREFIX=\"${XLINT_TARGET_PREFIX}\"

.include "${_CURDIR}/Makefile.inc"

${TIMESTAMP}: ${SRCS}
