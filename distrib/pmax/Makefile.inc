#	$NetBSD: Makefile.inc,v 1.3 2000/05/02 16:06:53 sjg Exp $

.include "../../Makefile.inc"

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
