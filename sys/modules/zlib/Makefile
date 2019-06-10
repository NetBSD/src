#	$NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:45 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/net

KMOD=	zlib
SRCS=	zlib.c

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.zlib.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
