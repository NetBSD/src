#	$NetBSD: Makefile,v 1.1 2006/09/21 01:27:32 reinoud Exp $

NOMAN=	# defined

PROG=	lseek

regress: ${PROG}
	@set -e; \
	if ./${PROG} /var/log/messages > /dev/null; then \
		echo PASSED; exit 0; \
	else \
		echo FAILED; exit 1; \
	fi

.include <bsd.prog.mk>
