#	$NetBSD: bsd.depall.mk,v 1.3 2001/05/14 03:20:10 sommerfeld Exp $

dependall: realdepend .MAKE
	@cd ${.CURDIR}; \
	${MAKE} realall
