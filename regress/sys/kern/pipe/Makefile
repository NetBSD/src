#	$NetBSD: Makefile,v 1.3 2001/12/12 01:24:18 tv Exp $

PROG=		pipe1
NOMAN=		# defined

regress: ${PROG}
	@./${PROG} || ( echo 'pipe write restart test failed' && exit 1 )

.include <bsd.prog.mk>
