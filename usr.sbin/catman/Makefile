#	$NetBSD: Makefile,v 1.11 2023/06/03 09:09:20 lukem Exp $

.include <bsd.own.mk>

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/man

COPTS.catman.c+= -Wno-format-nonliteral ${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
