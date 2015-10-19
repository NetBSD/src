#	$NetBSD: Makefile,v 1.9 2015/10/19 16:16:33 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../opencrypto

LIB=	rumpdev_opencrypto
COMMENT=OpenCrypto, incl. /dev/crypto

IOCONF=	OPENCRYPTO.ioconf
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
