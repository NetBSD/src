#	$NetBSD: Makefile,v 1.3 2002/09/18 05:41:42 lukem Exp $

NOMAN=		# defined

PROG=		ioctl
WARNS=		2
LDADD+=		-lutil
DPADD+=		${LIBUTIL}

regress: ${PROG}
	@if ./ioctl > /dev/null; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
