#	$NetBSD: Makefile.inc,v 1.1 2003/06/27 05:21:53 tshiozak Exp $

.PATH:	${ARCHDIR}/iconv ${.CURDIR}/iconv

SRCS+=	iconv.c

.if ${CITRUS} == "yes"
CPPFLAGS+= -DCITRUS_ICONV
.endif
