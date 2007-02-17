#	$NetBSD: Makefile,v 1.3 2007/02/17 18:30:29 hubertf Exp $

NOMAN=  	# defined
CLEANFILES=	input-utf8 result-general result-latin1 result-utf8

regress:
	@${HOST_SH} ${.CURDIR}/cut_tests ${.CURDIR}

.include <bsd.prog.mk>
