#	$NetBSD: Makefile.inc,v 1.2 2011/08/16 10:33:17 christos Exp $
.PATH.c: ${.PARSEDIR}
SRCS+=	snprintb.c
COPTS.snprintb.c = -Wno-format-nonliteral
