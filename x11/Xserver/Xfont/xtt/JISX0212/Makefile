#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:09 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	JISX0212

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/JISX0212
SRCS=	main.c JISX0212toUCS2.c

.include "../Makefile.xtt"

