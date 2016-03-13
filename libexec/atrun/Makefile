#	$NetBSD: Makefile,v 1.15 2016/03/13 00:32:09 dholland Exp $

.include <bsd.own.mk>

PROG=	atrun
SRCS=	atrun.c
BINDIR=	/usr/libexec
MAN=	atrun.8

.PATH.c: ${NETBSDSRCDIR}/usr.bin/at
SRCS+=	privs.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/at
COPTS.atrun.c = -Wno-format-nonliteral

.include <bsd.prog.mk>
