#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:10 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	KOI8

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/KOI8
SRCS=	main.c KOI8_RtoUCS2.c KOI8_UNItoUCS2.c

.include "../Makefile.xtt"

