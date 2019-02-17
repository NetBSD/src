#	$NetBSD: Makefile,v 1.3 2019/02/17 04:06:01 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	zlib
SRCS=	zlib.c

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
