#	$NetBSD: Makefile.inc,v 1.4.42.2 2020/04/21 19:37:51 martin Exp $
.PATH.c: ${.PARSEDIR}
SRCS+=	snprintb.c proc_compare.c getfstypename.c
COPTS.snprintb.c = -Wno-format-nonliteral
