#	$NetBSD: Makefile.inc,v 1.1.52.1 2006/09/09 02:40:22 rpaulo Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

