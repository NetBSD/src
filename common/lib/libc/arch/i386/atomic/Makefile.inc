#	$NetBSD: Makefile.inc,v 1.2 2007/11/28 01:33:47 ad Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic.S

.endif
