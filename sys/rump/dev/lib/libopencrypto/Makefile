#	$NetBSD: Makefile,v 1.6 2014/02/03 23:11:40 pgoyette Exp $
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

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
