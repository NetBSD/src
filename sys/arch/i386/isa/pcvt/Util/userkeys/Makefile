#	$NetBSD: Makefile,v 1.2 1994/10/27 04:20:42 cgd Exp $

.if !exists(${.CURDIR}/../Makefile.inc)
error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       ../Makefile.inc.NetBSD    or   ../Makefile.inc.FreeBSD"
	@echo
	@echo "             to ../Makefile.inc before you can proceed !"
	@echo
.else

PROG=	vt220keys
CLEANFILES+= core.vt220keys vt220keys.core

.include <bsd.prog.mk>

.endif
