#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:53 christos Exp $
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
