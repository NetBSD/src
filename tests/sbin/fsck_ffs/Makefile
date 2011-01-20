# $NetBSD: Makefile,v 1.1.2.1 2011/01/20 14:25:04 bouyer Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/fsck_ffs

.for name in t_enable_quotas t_check_quotas
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	quotas_common.sh ${name}.sh
.endfor

.include <bsd.test.mk>
