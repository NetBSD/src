#	$NetBSD: Makefile,v 1.3.4.2 2007/01/23 23:01:35 tron Exp $

SUBDIR=	acecad mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 usbtablet void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.include <bsd.subdir.mk>
