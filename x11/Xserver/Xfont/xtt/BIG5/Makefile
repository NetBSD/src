#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:09 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	BIG5

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/BIG5
SRCS=	main.c BIG5toUCS2.c

.include "../Makefile.xtt"

