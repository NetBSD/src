#	$NetBSD: Makefile.inc,v 1.3.198.1 2014/08/20 00:03:17 tls Exp $

BINDIR=		/usr/mdec

CPPFLAGS+=	-D_STANDALONE

CFLAGS+=	${${ACTIVE_CC} == "gcc":? -msoft-float :}
