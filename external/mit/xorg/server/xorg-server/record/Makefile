#	$NetBSD: Makefile,v 1.1 2008/08/02 21:19:21 mrg Exp $

.include "../Makefile.serverlib"

.if ${MKX11LOADABLE} != "no"
XMODULEDIR=	${X11ROOTDIR}/lib/modules/extensions
SRCS=		recordmod.c
.endif

.include "../Makefile.servermod"
.include "Makefile.record"
