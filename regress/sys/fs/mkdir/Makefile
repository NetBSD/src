#	$NetBSD: Makefile,v 1.1 2002/01/27 21:49:46 thorpej Exp $

PROG=	mkdir
NOMAN=	# defined

regress: ${PROG}
	@./${PROG} || (echo 'mkdir test failed' && exit 1)

.include <bsd.prog.mk>
