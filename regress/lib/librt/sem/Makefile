#	$NetBSD: Makefile,v 1.1 2003/01/24 01:56:48 thorpej Exp $

WARNS?=1
NOMAN=		# defined

PROG=		sem
LDADD+=		-lrt

regress:
	@echo Testing POSIX 1003.1b semaphores
	./sem

.include <bsd.prog.mk>
