#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:10 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	MULEENCODING

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/MULEENCODING
SRCS=	main.c MULELAOtoUCS2.c ASCIILAOtoUCS2.c

.include "../Makefile.xtt"

