#	$NetBSD: Makefile,v 1.3.28.1 2026/11/04 22:52:23 perseant Exp $

LIBISPRIVATE=   yes

LIB=	backtrace

.include "Makefile.sources"

SRCS=	${BACKTRACE_SRCS}

.for _s in ${BACKTRACE_SRCS}
CPPFLAGS.${_s}+=	${BACKTRACE_CPPFLAGS}
.endfor

.include <bsd.lib.mk>

.PATH: ${GCCDIST}/libbacktrace
