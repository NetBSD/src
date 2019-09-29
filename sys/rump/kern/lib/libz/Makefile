#	$NetBSD: Makefile,v 1.4 2019/09/29 23:45:00 mrg Exp $
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
