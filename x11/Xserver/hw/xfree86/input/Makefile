#	$NetBSD: Makefile,v 1.3 2004/07/13 09:29:45 sekiya Exp $

SUBDIR=	mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.include <bsd.subdir.mk>
