#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:39 rtr Exp $

NOLINT=	1 
.include <bsd.own.mk>

.if ${X11LOADABLE} != "no"
PIXELSIZE=	32
XMODULEDIR=	${X11USRLIBDIR}/modules
SRCS=		cfbmodule32.c
.endif

.include "../Makefile.serverlib"
.include "../cfb/Makefile.cfb"
