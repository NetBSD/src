#	$NetBSD: Makefile.inc,v 1.1.56.1 2006/05/24 10:56:52 yamt Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

