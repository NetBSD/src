#	$NetBSD: Makefile.inc,v 1.4 2000/06/01 06:53:55 matt Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
