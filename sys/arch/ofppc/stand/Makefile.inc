#	$NetBSD: Makefile.inc,v 1.4 2014/08/10 17:44:26 joerg Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-D_STANDALONE

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
