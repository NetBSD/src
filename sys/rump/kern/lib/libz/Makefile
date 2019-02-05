#	$NetBSD: Makefile,v 1.3 2019/02/05 09:29:20 mrg Exp $
#

# zlib.c is logically in sys/net
.PATH:	${.CURDIR}/../../../../net

LIB=	rumpkern_z
COMMENT=Data compression

SRCS=	zlib.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif
