#	$NetBSD: Makefile,v 1.1 2002/04/09 06:10:44 jdolecek Exp $

PROG=		writev
NOMAN=		# defined

regress:	${PROG}
	@echo "This should print 'writev (expected error): Invalid argument'"
	@echo "Broken if it hangs" 
	./${PROG}

.include <bsd.prog.mk>
