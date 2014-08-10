# $NetBSD: Makefile,v 1.1.10.1 2014/08/10 06:57:21 tls Exp $

.include <bsd.own.mk>
.include <../../../external/bsd/sljit/Makefile.inc>

TESTSDIR=	${TESTSBASE}/lib/libbpfjit

TESTS_C+=	t_bpfjit
TESTS_C+=	t_extmem
TESTS_C+=	t_cop

# XXX this variable doesn't belong to here
LIBBPFJITDIR!=	cd ${NETBSDSRCDIR}/lib/libbpfjit && ${PRINTOBJDIR}

LDADD+=		-L${LIBBPFJITDIR} -lbpfjit
DPADD+=		${LIBBPFJITDIR}/libbpfjit.a

LDADD+=		-L${LIBSLJITDIR} -lsljit
DPADD+=		${LIBSLJITDIR}/libsljit.a

LDADD+=		${LIBPCAP}

.include <bsd.test.mk>
