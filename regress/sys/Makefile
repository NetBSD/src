#	$NetBSD: Makefile,v 1.5 1997/05/07 16:01:44 mycroft Exp $

SUBDIR+= kern
.if exists(arch/${MACHINE})
SUBDIR+= arch/${MACHINE}
.endif

regress: _SUBDIRUSE

.include <bsd.subdir.mk>
