#	$NetBSD: Makefile,v 1.1 2002/02/12 06:00:48 simonb Exp $

NOMAN=		# defined

regress:
	sh ${.CURDIR}/test.sh -r | diff - ${.CURDIR}/test.out

.include <bsd.prog.mk>
