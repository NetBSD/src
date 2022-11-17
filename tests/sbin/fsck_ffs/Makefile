# $NetBSD: Makefile,v 1.3 2022/11/17 06:40:40 chs Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/sbin/fsck_ffs

.for name in t_enable_quotas t_check_quotas
TESTS_SH+=	${name}
TESTS_SH_SRC_${name}=	quotas_common.sh ${name}.sh
.endfor

TESTS_SH+=	t_extattr

.include <bsd.test.mk>
