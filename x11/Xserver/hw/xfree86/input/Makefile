#	$NetBSD: Makefile,v 1.3.2.2 2004/12/07 11:00:33 rtr Exp $

.include "../../../Makefile.serverlib"

SUBDIR=	mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.if ${MKX11LOADABLE} != "no"
.include <bsd.subdir.mk>
.endif
