#	$NetBSD: Makefile.inc,v 1.4.2.1 2002/03/25 17:33:01 he Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh 
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh

.if target(install) && !defined(RELEASEDIR)
beforeinstall:
	@echo setenv RELEASEDIR before doing that!
	@false
.endif
