#	$NetBSD: Makefile.inc,v 1.5 2006/05/12 16:45:12 dogcow Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

