#	$NetBSD: Makefile,v 1.2 2004/03/03 08:38:09 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/codeconv

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	ISO8859_1

.PATH:	${X11SRCDIR.xc}/extras/X-TrueType/ISO8859.1
SRCS=	main.c ISO8859_1toAROMAN.c

.include "../Makefile.xtt"

