#	$NetBSD: Makefile,v 1.6 2004/06/26 22:11:49 dsl Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh
	sh ${.CURDIR}/fsplit.sh

.include <bsd.prog.mk>
