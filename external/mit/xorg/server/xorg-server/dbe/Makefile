#	$NetBSD: Makefile,v 1.4 2016/08/14 03:43:04 mrg Exp $

LIBISPRIVATE=	yes

.include "../Makefile.serverlib"

LIB=	dbe
SRCS=	${SRCS.dbe}

CPPFLAGS+=      -I${X11SRCDIR.xorg-server}/present

.include "Makefile.dbe"
