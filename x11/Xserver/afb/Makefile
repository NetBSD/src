#	$NetBSD: Makefile,v 1.4.8.3 2005/07/03 17:05:22 snj Exp $

.include <bsd.x11.mk>

.if defined(X11FLAGS.LOADABLE)
SUBDIR+=	afbmod
.else
SUBDIR+=	afb
.endif

.include <bsd.subdir.mk>

gaga:
	@echo ${SUBDIR}
