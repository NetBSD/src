#	$NetBSD: Makefile,v 1.3 2014/01/16 02:38:20 pgoyette Exp $
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

# deflate.c also requires zlib, so include the appropriate library

LDADD+=	-lrumpkern_z

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
