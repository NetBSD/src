#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:53 lukem Exp $

NOMAN=	# defined

PROG=	mkdir

regress: ${PROG}
	@./${PROG} || (echo 'mkdir test failed' && exit 1)

.include <bsd.prog.mk>
