#	$NetBSD: Makefile,v 1.2 2001/10/24 02:32:56 thorpej Exp $

XLINT_TARGET_PREFIX=${MACHINE_GNU_PLATFORM}-

HOSTPROGNAME=	${XLINT_TARGET_PREFIX}lint
HOST_SRCDIR=	usr.bin/xlint/xlint
HOST_CPPFLAGS=	-DPATH_LIBEXEC=\"${TOOLDIR}/libexec\"

.include "${.CURDIR}/../Makefile.host"

${TIMESTAMP}: ${SRCS}
