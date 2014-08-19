#	$NetBSD: Makefile,v 1.7.10.2 2014/08/20 00:04:37 tls Exp $
#

.PATH:	${.CURDIR}/../../../../opencrypto

LIB=	rumpdev_opencrypto

SRCS=	opencrypto_component.c

# Sources for crypto framework

SRCS+=	criov.c
SRCS+=	crypto.c
SRCS+=	xform.c

# Sources for crypto device, for userland access

SRCS+=	cryptodev.c
SRCS+=	ocryptodev.c

# Sources for swcrypto device to access opencrypto software algorithms
# (Other algorithms are in rumpkern_crypto)

SRCS+=	aesxcbcmac.c
SRCS+=	cryptosoft.c
SRCS+=	deflate.c
SRCS+=	gmac.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
