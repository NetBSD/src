#	$NetBSD: Makefile,v 1.1 2013/11/28 22:33:43 christos Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppdump ${NETBSDSRCDIR}/sys/net

PROG=	pppdump
SRCS=	pppdump.c deflate.c bsd-comp.c zlib.c
MAN=	pppdump.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/net -I${.CURDIR}
CPPFLAGS+=	-DDO_BSD_COMPRESS -DDO_DEFLATE

.include <bsd.prog.mk>
