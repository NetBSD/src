# $NetBSD: Makefile,v 1.6 2020/10/01 13:49:18 rin Exp $
#

.include <bsd.own.mk>

PROG=			rump_open_tap
MAN=			# empty
DPADD=			${LIBRUMPRES} ${LIBRUMPCLIENT}
LDADD=			-lrumpres -lrumpclient
BINDIR.rump_open_tap=	${TESTSDIR}

TESTSDIR=		${TESTSBASE}/net/if_tap

.for name in tap
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

.include <bsd.test.mk>
