#	$NetBSD: Makefile,v 1.8 2002/09/18 03:54:28 lukem Exp $

.include <bsd.own.mk>

BINDIR=		/usr/sbin
PROG=		catman
SRCS=		catman.c manconf.c
MAN=		catman.8
.PATH:		${NETBSDSRCDIR}/usr.bin/man
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/man

.include <bsd.prog.mk>
