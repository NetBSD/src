#	$NetBSD: Makefile,v 1.7 2005/07/03 14:30:52 tron Exp $

.include <bsd.x11.mk>

.if defined(X11FLAGS.LOADABLE)
SUBDIR+=	afbmod
.else
SUBDIR+=	afb
.endif

.include <bsd.subdir.mk>

gaga:
	@echo ${SUBDIR}
