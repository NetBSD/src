#	$NetBSD: Makefile,v 1.3 2005/02/21 06:41:20 cube Exp $

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppdump

PROG=	pppdump
SRCS=	pppdump.c deflate.c bsd-comp.c
MAN=	pppdump.8

CPPFLAGS+=	-I${.CURDIR}

DPADD+=${LIBZ}
LDADD+=-lz
.include <bsd.prog.mk>
