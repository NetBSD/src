#	$NetBSD: bsd.depall.mk,v 1.2 2000/01/22 19:31:01 mycroft Exp $

dependall: realdepend
	@cd ${.CURDIR}; \
	${MAKE} realall
