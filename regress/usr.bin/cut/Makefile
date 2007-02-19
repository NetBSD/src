#	$NetBSD: Makefile,v 1.4 2007/02/19 11:15:47 cube Exp $

NOMAN=  	# defined
CLEANFILES=	result-general result-latin1 result-utf8

regress:
	@${HOST_SH} ${.CURDIR}/cut_tests ${.CURDIR}

.include <bsd.prog.mk>
