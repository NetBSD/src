#	$NetBSD: Makefile,v 1.2.2.1 2004/08/12 04:08:37 jmc Exp $

SUBDIR=	mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.include <bsd.subdir.mk>
