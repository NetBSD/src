#	$NetBSD: Makefile.inc,v 1.2 1999/04/27 12:05:05 simonb Exp $

REV!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
