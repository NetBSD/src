#	$NetBSD: Makefile,v 1.4 2002/09/18 05:41:54 lukem Exp $

NOMAN=		# defined

PROG=		pipe1

regress: ${PROG}
	@./${PROG} || ( echo 'pipe write restart test failed' && exit 1 )

.include <bsd.prog.mk>
