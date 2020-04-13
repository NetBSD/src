#	$NetBSD: Makefile,v 1.2.18.2 2020/04/13 08:05:19 martin Exp $
#

# zlib.c is logically in sys/net
.PATH:	${.CURDIR}/../../../../net

LIB=	rumpkern_z
COMMENT=Data compression

SRCS=	zlib.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif
