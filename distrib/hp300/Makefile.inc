#	$NetBSD: Makefile.inc,v 1.1 2002/02/10 01:22:54 gmcgarry Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
