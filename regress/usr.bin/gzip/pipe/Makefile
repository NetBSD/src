#	$NetBSD: Makefile,v 1.1 2005/02/21 20:08:18 yamt Exp $

NOMAN=  	# defined

.include <bsd.own.mk>

regress:
	dd if=/dev/zero count=102400 2> /dev/null | ${GZIP} -c | \
	dd bs=1 2> /dev/null | ${GZIP} -tc

.include <bsd.prog.mk>
