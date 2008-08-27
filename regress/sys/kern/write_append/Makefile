#	$NetBSD: Makefile,v 1.1 2008/08/27 06:40:13 christos Exp $

NOMAN=		# defined

PROG=		write_append
WARNS?=		4

regress:
	@if ${.OBJDIR}/${PROG} 2> /dev/null; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
