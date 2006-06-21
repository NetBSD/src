#	$NetBSD: Makefile.inc,v 1.1.38.1 2006/06/21 14:52:44 yamt Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

