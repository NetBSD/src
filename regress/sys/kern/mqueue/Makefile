#	$NetBSD: Makefile,v 1.2 2009/07/19 02:31:19 rmind Exp $
#
PROG=		mq_prio_test
NOMAN=		# defined
WARNS=		4
LDADD=		-lrt

regress:
	@/sbin/sysctl -w kern.mqueue.mq_prio_max=33
	@if ./$(PROG); then \
		echo "PASSED"; \
	else \
		echo "FAILED"; \
	fi

.include <bsd.prog.mk>
