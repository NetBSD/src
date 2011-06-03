# $NetBSD: Makefile,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $

NOMAN=		#defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel/port-xen


TESTS_C=	t_xen_ipi t_tlb_thrash
SRCS.t_tlb_thrash= arrayalloc.c system.c thread.c barriers.c t_tlb_thrash.c
LDADD+=		-lpthread

.include <bsd.test.mk>
