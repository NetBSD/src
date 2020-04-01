# $NetBSD: Makefile,v 1.7 2020/04/01 01:49:26 christos Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/mpls

.for name in t_mpls_fw t_mpls_fw6 t_mpls_fw64 t_rfc4182 t_ldp_regen
TESTS_SH+=              ${name}
TESTS_SH_SRC_${name}= mpls_common.sh ${name}.sh
.endfor

.include <bsd.test.mk>
