#	$NetBSD: Makefile.inc,v 1.4.42.1 2021/04/17 17:26:14 thorpej Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-D_STANDALONE
COPTS+=		-fcommon

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
