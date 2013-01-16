# $NetBSD: Makefile,v 1.1.4.2 2013/01/16 05:33:59 yamt Exp $

.include <bsd.own.mk>
.include <../../../external/bsd/sljit/Makefile.inc>

TESTSDIR=	${TESTSBASE}/lib/libbpfjit

TESTS_C+=	t_bpfjit

# XXX this variable doesn't belong to here
LIBBPFJITDIR!=	cd ${NETBSDSRCDIR}/lib/libbpfjit && ${PRINTOBJDIR}

LDADD+=		-L${LIBBPFJITDIR} -lbpfjit
DPADD+=		${LIBBPFJITDIR}/libbpfjit.a

LDADD+=		-L${LIBSLJITDIR} -lsljit
DPADD+=		${LIBSLJITDIR}/libsljit.a

LDADD+=		${LIBPCAP}

.include <bsd.test.mk>
