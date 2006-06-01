#	$NetBSD: Makefile.inc,v 1.1.54.1 2006/06/01 22:34:53 kardel Exp $

.include <bsd.own.mk>

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif

