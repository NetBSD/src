#	$NetBSD: Makefile.inc,v 1.1.2.1 1999/10/20 23:03:53 he Exp $

REV!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
