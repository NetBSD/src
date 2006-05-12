#	$NetBSD: Makefile.inc,v 1.4 2006/05/12 06:43:55 dogcow Exp $

BINDIR=	/usr/mdec

.if (${HAVE_GCC:U0} == 4)
CFLAGS+=	-Wno-pointer-sign
.endif

