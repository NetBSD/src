#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:39 rtr Exp $

NOLINT=		1 # midbe.c
.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules/extensions
SRCS=		dbemodule.c
.endif

.include "../Makefile.serverlib"
.include "Makefile.dbe"
