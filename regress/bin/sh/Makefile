#	$NetBSD: Makefile,v 1.10 2006/05/12 00:05:59 rillig Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh
	sh ${.CURDIR}/fsplit.sh
	sh ${.CURDIR}/here.sh
	sh ${.CURDIR}/set_e.sh
	#sh ${.CURDIR}/expand.sh	# First needs to be fixed.

.include <bsd.prog.mk>
