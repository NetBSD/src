#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:44 rtr Exp $

.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions
SRCS=		recordmod.c
.endif

.include "../Makefile.serverlib"
.include "Makefile.record"
