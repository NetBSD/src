#	$NetBSD: Makefile.inc,v 1.3 2011/10/21 02:09:00 christos Exp $
.PATH.c: ${.PARSEDIR}
SRCS+=	snprintb.c proc_compare.c
COPTS.snprintb.c = -Wno-format-nonliteral
