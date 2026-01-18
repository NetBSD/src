#	$NetBSD: Makefile,v 1.4 2026/01/18 08:07:06 mrg Exp $

LIBISPRIVATE=   yes

LIB=	backtrace

.include "Makefile.sources"

SRCS=	${BACKTRACE_SRCS}

.for _s in ${BACKTRACE_SRCS}
CPPFLAGS.${_s}+=	${BACKTRACE_CPPFLAGS}
.endfor

.include <bsd.lib.mk>

.PATH: ${GCCDIST}/libbacktrace
