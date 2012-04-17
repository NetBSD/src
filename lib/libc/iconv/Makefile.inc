#	$NetBSD: Makefile.inc,v 1.4.8.1 2012/04/17 00:05:19 yamt Exp $

.PATH:	${ARCHDIR}/iconv ${.CURDIR}/iconv

SRCS+=	iconv.c
MAN+=	iconv.3
MLINKS+=iconv.3 iconv_open.3 iconv.3 iconv_close.3

CPPFLAGS.iconv.c+=	-I${LIBCDIR}/citrus
