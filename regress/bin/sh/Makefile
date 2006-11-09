#	$NetBSD: Makefile,v 1.11 2006/11/09 12:17:35 rillig Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh
	sh ${.CURDIR}/fsplit.sh
	sh ${.CURDIR}/here.sh
	sh ${.CURDIR}/set_e.sh
	sh ${.CURDIR}/expand.sh

.include <bsd.prog.mk>
