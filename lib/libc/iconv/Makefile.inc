#	$NetBSD: Makefile.inc,v 1.2 2003/07/04 06:05:30 tshiozak Exp $

.PATH:	${ARCHDIR}/iconv ${.CURDIR}/iconv

SRCS+=	iconv.c
MAN+=	iconv.3
MLINKS+=iconv.3 iconv_open.3 iconv.3 iconv_close.3

.if ${CITRUS} == "yes"
CPPFLAGS+= -DCITRUS_ICONV
.endif
