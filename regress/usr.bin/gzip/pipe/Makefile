#	$NetBSD: Makefile,v 1.2 2006/05/29 16:47:07 simonb Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

regress:
	dd if=/dev/zero count=102400 2> /dev/null | ${GZIPPROG} -c | \
	dd bs=1 2> /dev/null | ${GZIPPROG} -tc

.include <bsd.prog.mk>
