#	$NetBSD: Makefile.inc,v 1.6 2026/10/04 03:28:44 mrg Exp $

.PATH:	${ARCHDIR}/iconv ${.PARSEDIR}

SRCS+=	iconv.c
MAN+=	iconv.3
MLINKS+=iconv.3 iconv_open.3 iconv.3 iconv_close.3

CPPFLAGS.iconv.c+=	-I${LIBCDIR}/citrus
