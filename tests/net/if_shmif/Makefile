# $NetBSD: Makefile,v 1.1.2.2 2024/08/24 16:42:25 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_shmif

.for name in shmif
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
