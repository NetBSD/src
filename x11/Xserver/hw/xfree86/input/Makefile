#	$NetBSD: Makefile,v 1.5 2005/11/12 22:57:46 spz Exp $

SUBDIR=	acecad mouse calcomp citron digitaledge dmc dynapro elographics fpit \
	hyperpen js_x kbd magellan microtouch mutouch palmax penmount \
	spaceorb summa tek4957 usbtablet void wacom

.if ${MACHINE} == "sgimips"
SUBDIR=	mouse kbd void
.endif

.include <bsd.subdir.mk>
