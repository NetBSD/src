#	$NetBSD: Makefile,v 1.1 2003/01/20 20:12:18 christos Exp $

NOMAN=		# defined

PROG=		sem
LDADD+=		-lpthread

regress:
	@echo Testing POSIX 1003.1b semaphores
	./sem

.include <bsd.prog.mk>
