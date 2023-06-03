#	$NetBSD: Makefile,v 1.5 2023/06/03 21:30:24 lukem Exp $
#

# zlib.c is logically in sys/net
.PATH:	${.CURDIR}/../../../../net

LIB=	rumpkern_z
COMMENT=Data compression

SRCS=	zlib.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

COPTS.zlib.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
