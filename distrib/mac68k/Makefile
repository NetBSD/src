#	$NetBSD: Makefile,v 1.5 2002/03/04 16:43:08 shiba Exp $

SUBDIR=	instkernel # miniroot

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release:
	cd ${.CURDIR}/instkernel/instkernel && ${MAKE} release
.endif
