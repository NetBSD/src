#	$NetBSD: bsd.depall.mk,v 1.1 1999/09/14 01:31:11 perry Exp $

dependall: depend
	@cd ${.CURDIR}; \
	${MAKE} all
