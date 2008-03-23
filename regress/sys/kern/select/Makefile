#	$NetBSD: Makefile,v 1.1.2.2 2008/03/23 00:43:00 matt Exp $

NOMAN=		# defined

PROG=		select
WARNS?=		4

LDADD=		-lpthread

regress:
	@if ${.OBJDIR}/select 5; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
