#	$NetBSD: Makefile,v 1.5 2003/10/26 07:25:34 lukem Exp $

NOMAN=  	# defined

# XXX: can't use ${HOST_SH} here.  this really should run on the target
regress:
	sh ${.CURDIR}/compexit.sh
	sh ${.CURDIR}/exitstatus.sh
	sh ${.CURDIR}/varquote.sh

.include <bsd.prog.mk>
