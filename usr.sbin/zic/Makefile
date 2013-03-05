#	$NetBSD: Makefile,v 1.13 2013/03/05 13:42:04 apb Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

COPTS.zic.c	+= -Wno-format-nonliteral
COPTS.scheck.c	+= -Wno-format-nonliteral

.if defined(HAVE_GCC) && ${HAVE_GCC} < 45
# gcc-4.1 says:
# "warning: comparison is always false due to limited range of data type"
# and there is no -Wno-foo option to suppress that warning.
#
COPTS.zic.c+= ${${ACTIVE_CC} == "gcc" :? -Wno-error :}
.endif # HAVE_GCC < 45

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
