#	$NetBSD: Makefile,v 1.1 2009/07/13 02:55:42 rmind Exp $
#
PROG=		mq_prio_test
NOMAN=		# defined
WARNS=		4
LDADD=		-lrt

regress:
	@/sbin/sysctl kern.mqueue.mq_prio_max=33
	@if ./$(PROG); then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
