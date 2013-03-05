#	$NetBSD: Makefile,v 1.12 2013/03/05 10:19:58 martin Exp $

.include "Makefile.inc"

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 45
COPTS.zic.c	+= -Wno-format-nonliteral
COPTS.scheck.c	+= -Wno-format-nonliteral
.else
# warning with older gcc:
# "comparison is always false due to limited range of data type"
# and no way to make it shut up?
WARNS=         0
NOGCCERROR=    1
.endif

.include <bsd.own.mk>

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
