#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		xf86XTrapModule.c
CPPFLAGS+=	-DEXTMODULE
.endif

.include "../Makefile.serverlib"
.include "Makefile.xtrap"
