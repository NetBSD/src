#	$NetBSD: Makefile.inc,v 1.1.60.1 2006/05/24 15:47:58 tron Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

