#	$NetBSD: Makefile,v 1.3.4.1 2005/04/06 21:37:57 tron Exp $

SUBDIR=	acecad mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.include <bsd.subdir.mk>
