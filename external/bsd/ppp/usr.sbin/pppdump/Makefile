#	$NetBSD: Makefile,v 1.1.42.1 2025/08/02 05:23:19 perseant Exp $

.include "../Makefile.inc"

.PATH: ${DIST}/pppdump ${NETBSDSRCDIR}/sys/net

PROG=	pppdump
SRCS=	pppdump.c zlib.c
MAN=	pppdump.8

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/net -I${.CURDIR}
CPPFLAGS+=	-DDO_BSD_COMPRESS -DDO_DEFLATE

.include <bsd.prog.mk>
