#	$NetBSD: Makefile,v 1.2 2005/02/20 11:12:46 cube Exp $

.PATH: ${NETBSDSRCDIR}/dist/pppd/pppdump

PROG=	pppdump
SRCS=	pppdump.c deflate.c bsd-comp.c
MAN=	pppdump.8

CPPFLAGS+=	-I${.CURDIR}

DPADD+=${LIBZ}
LDADD+=-lz
.include <bsd.prog.mk>
