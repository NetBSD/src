# $NetBSD: Makefile,v 1.3 2020/09/30 14:43:15 roy Exp $
#

.include <bsd.own.mk>

PROG=		rump_open_tap
MAN=
DPADD=		${LIBRUMPRES}
LDADD=		-lrumpres

TESTSDIR=	${TESTSBASE}/net/if_tap

.for name in tap
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
