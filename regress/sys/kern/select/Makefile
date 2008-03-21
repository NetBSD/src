#	$NetBSD: Makefile,v 1.1 2008/03/21 12:27:12 yamt Exp $

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
