# $NetBSD: Makefile.inc,v 1.1.98.1 2026/11/04 23:19:18 perseant Exp $

# hash functions
.PATH: ${.PARSEDIR}

SRCS+=	md2.c md2hl.c

MAN+=	md2.3

MLINKS+=md2.3 MD2Init.3 md2.3 MD2Update.3 md2.3 MD2Final.3
MLINKS+=md2.3 MD2End.3  md2.3 MD2File.3   md2.3 MD2Data.3
MLINKS+=md2.3 MD2Transform.3  md2.3 MD2FileChunk.3
