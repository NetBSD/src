#	$NetBSD: Makefile.inc,v 1.5.52.1 2026/11/04 23:19:19 perseant Exp $

.PATH:	${ARCHDIR}/iconv ${.PARSEDIR}

SRCS+=	iconv.c
MAN+=	iconv.3
MLINKS+=iconv.3 iconv_open.3 iconv.3 iconv_close.3

CPPFLAGS.iconv.c+=	-I${LIBCDIR}/citrus
