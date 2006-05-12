#	$NetBSD: Makefile.inc,v 1.3 2006/05/12 06:08:47 dogcow Exp $

BINDIR=	/usr/mdec

.if ${HAVE_GCC} == 4
CFLAGS+=	-Wno-pointer-sign
.endif
