#	$NetBSD: Makefile.inc,v 1.5 2021/04/14 09:29:40 mrg Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-D_STANDALONE
COPTS+=		-fcommon

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
