#	$NetBSD: Makefile,v 1.1 2014/01/14 17:15:09 pgoyette Exp $
#

.PATH:	${.CURDIR}/../../../../opencrypto

LIB=	rumpdev_opencrypto

SRCS=	component.c

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

.PATH:	${.CURDIR}/../../../../net

SRCS+=	zlib.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs
CPPFLAGS+=	-DCOMPAT_50

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
