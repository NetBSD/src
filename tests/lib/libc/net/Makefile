# $NetBSD: Makefile,v 1.6 2011/11/01 22:36:53 pgoyette Exp $

.include <bsd.own.mk>

MKMAN=	no

TESTS_SUBDIRS+=	getaddrinfo

TESTSDIR=	${TESTSBASE}/lib/libc/net

TESTS_C+=	t_getprotoent
TESTS_C+=	t_ether_aton

SRCS.t_ether_aton=	aton_ether_subr.c t_ether_aton.c

aton_ether_subr.c: gen_ether_subr ${NETBSDSRCDIR}/sys/net/if_ethersubr.c
	${HOST_SH} ${.ALLSRC} ${.TARGET}

TESTS_SH+=	t_nsdispatch
TESTS_SH+=	t_protoent
TESTS_SH+=	t_servent

BINDIR=		${TESTSDIR}

PROGS+=		h_nsd_recurse
PROGS+=		h_protoent
PROGS+=		h_servent

LDADD.h_nsd_recurse+=	-lpthread

CLEANFILES+=	aton_ether_subr_.c

.include <bsd.test.mk>
