#	$NetBSD: Makefile,v 1.2 2001/12/12 01:24:05 tv Exp $

PROG=		ioctl
NOMAN=		# defined
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
