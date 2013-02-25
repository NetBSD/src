# $NetBSD: Makefile,v 1.4.12.1 2013/02/25 00:30:21 tls Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/ipf

# t_bpf is omitted from the test list since we don't support it on NetBSD

.for name in t_filter_exec t_filter_parse t_logging t_nat_exec \
             t_nat_ipf_exec t_nat_parse t_pools
TESTS_SH+=		${name}
TESTS_SH_SRC_${name}=	h_common.sh ${name}.sh
.endfor

SUBDIR=		regress input expected

.include <bsd.test.mk>
