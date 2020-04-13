#	$NetBSD: Makefile,v 1.9.42.1 2020/04/13 08:05:51 martin Exp $

.include <bsd.own.mk>

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/man

COPTS.catman.c+= -Wno-format-nonliteral ${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
