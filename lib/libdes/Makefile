#	$NetBSD: Makefile,v 1.3 2000/06/19 19:39:09 thorpej Exp $

.BEGIN:
.if !make(print-shlib-major) && !make(print-shlib-minor)
	@echo "This is a dummy directory; don't build in here."
	@false
.endif

.include <bsd.lib.mk>
