#	$NetBSD: Makefile,v 1.10 2019/10/13 07:28:18 mrg Exp $

.include <bsd.own.mk>

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/man

COPTS.catman.c+= -Wno-format-nonliteral ${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
