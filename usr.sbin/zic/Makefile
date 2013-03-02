#	$NetBSD: Makefile,v 1.11 2013/03/02 21:38:19 christos Exp $

.include "Makefile.inc"

COPTS.zic.c	+= -Wno-format-nonliteral
COPTS.scheck.c	+= -Wno-format-nonliteral

.include <bsd.own.mk>

PROG=	zic
SRCS=	zic.c scheck.c ialloc.c
MAN=	zic.8
CPPFLAGS+=-Dunix

.PATH: ${NETBSDSRCDIR}/lib/libc/time

.include <bsd.prog.mk>
