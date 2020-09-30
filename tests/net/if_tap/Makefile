# $NetBSD: Makefile,v 1.5 2020/09/30 17:14:11 roy Exp $
#

.include <bsd.own.mk>

PROG=			rump_open_tap
MAN=			# empty
DPADD=			${LIBRUMPRES}
LDADD=			-lrumpres
BINDIR.rump_open_tap=	${TESTSDIR}

TESTSDIR=		${TESTSBASE}/net/if_tap

.for name in tap
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
