#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:09 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	ARABIC

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/ARABIC
SRCS=	main.c MULEARABICtoUCS2.c CP1256toUCS2.c XATERMtoUCS2.c

.include "../Makefile.xtt"

