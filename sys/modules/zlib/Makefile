#	$NetBSD: Makefile,v 1.1.36.2 2020/04/13 08:05:14 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	zlib
SRCS=	zlib.c

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
