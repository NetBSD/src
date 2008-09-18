#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 04:38:42 wrstuden Exp $

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
