#	$NetBSD: Makefile,v 1.2 2002/09/18 05:41:56 lukem Exp $

NOMAN=		# defined

PROG=		writev

regress:	${PROG}
	@echo "This should print 'writev (expected error): Invalid argument'"
	@echo "Broken if it hangs" 
	./${PROG}

.include <bsd.prog.mk>
