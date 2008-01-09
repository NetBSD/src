#	$NetBSD: Makefile.inc,v 1.2.4.2 2008/01/09 01:21:14 matt Exp $

.if defined(LIB) && (${LIB} == "kern")

SRCS+=	atomic.S

.endif
