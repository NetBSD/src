#	$NetBSD: Makefile,v 1.7 2004/06/27 10:31:06 dsl Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh
	sh ${.CURDIR}/fsplit.sh
	sh ${.CURDIR}/here.sh

.include <bsd.prog.mk>
