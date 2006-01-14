#	$NetBSD: Makefile,v 1.4 2006/01/14 19:55:11 christos Exp $

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppdump ${NETBSDSRCDIR}/sys/net

PROG=	pppdump
SRCS=	pppdump.c deflate.c bsd-comp.c zlib.c
MAN=	pppdump.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/net -I${.CURDIR}

.include <bsd.prog.mk>
