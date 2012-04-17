#	$NetBSD: Makefile.inc,v 1.3.2.1 2012/04/17 00:01:40 yamt Exp $
.PATH.c: ${.PARSEDIR}
SRCS+=	snprintb.c proc_compare.c getfstypename.c
COPTS.snprintb.c = -Wno-format-nonliteral
