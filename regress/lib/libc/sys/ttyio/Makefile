#	$NetBSD: Makefile,v 1.1 2001/09/20 16:56:53 atatat Exp $

PROG=	ioctl
MKMAN=	no
WARNS=	2
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

regress: ${PROG}
	@if ./ioctl > /dev/null; then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
