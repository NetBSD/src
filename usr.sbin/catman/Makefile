#	$NetBSD: Makefile,v 1.9 2011/08/16 08:04:17 christos Exp $

.include <bsd.own.mk>

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/man

COPTS.catman.c += -Wno-format-nonliteral

.include <bsd.prog.mk>
