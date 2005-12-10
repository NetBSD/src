#	$NetBSD: Makefile,v 1.9 2005/12/10 14:22:17 yamt Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh
	sh ${.CURDIR}/fsplit.sh
	sh ${.CURDIR}/here.sh
	sh ${.CURDIR}/set_e.sh

.include <bsd.prog.mk>
