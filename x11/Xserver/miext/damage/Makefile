#	$NetBSD: Makefile,v 1.1.2.2 2004/11/21 10:01:28 rtr Exp $

.include "../../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
XMODULEDIR=	${X11USRLIBDIR}/modules
.endif

.include "../../Makefile.servermod"
.include "Makefile.damage"
