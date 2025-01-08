#	$NetBSD: Makefile,v 1.2 2025/01/08 19:59:41 christos Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppdump ${NETBSDSRCDIR}/sys/net

PROG=	pppdump
SRCS=	pppdump.c zlib.c
MAN=	pppdump.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/net -I${.CURDIR}
CPPFLAGS+=	-DDO_BSD_COMPRESS -DDO_DEFLATE

.include <bsd.prog.mk>
