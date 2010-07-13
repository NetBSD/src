# $NetBSD: Makefile,v 1.3 2010/07/13 21:13:25 jmmv Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/ipf

.for name in t_bpf t_filter_exec t_filter_parse t_logging t_nat_exec \
             t_nat_ipf_exec t_nat_parse t_pools
TESTS_SH+=		${name}
TESTS_SH_SRC_${name}=	h_common.sh ${name}.sh
.endfor

SUBDIR=		regress input expected

.include <bsd.test.mk>
