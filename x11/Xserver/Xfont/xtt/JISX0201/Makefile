#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:09 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	JISX0201

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/JISX0201
SRCS=	main.c JISX0201toUCS2.c

.include "../Makefile.xtt"

