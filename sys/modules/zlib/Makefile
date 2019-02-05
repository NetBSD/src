#	$NetBSD: Makefile,v 1.2 2019/02/05 09:29:20 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	zlib
SRCS=	zlib.c

.include <bsd.kmodule.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif
