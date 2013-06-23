#	$NetBSD: Makefile,v 1.9.12.2 2013/06/23 06:29:06 tls Exp $

.include "Makefile.inc"
.include <bsd.own.mk>

COPTS.zic.c	+= -Wno-format-nonliteral
COPTS.scheck.c	+= -Wno-format-nonliteral

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
