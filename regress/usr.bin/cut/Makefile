#	$NetBSD: Makefile,v 1.2 2007/02/17 18:27:55 hubertf Exp $

NOMAN=  	# defined
CLEANFILES=	input-utf8 result-general

regress: input-utf8
	@${HOST_SH} ${.CURDIR}/cut_tests ${.CURDIR}

input-utf8:
	@iconv -f latin1 -t utf-8 ${.CURDIR}/input-latin1 > input-utf8

.include <bsd.prog.mk>
