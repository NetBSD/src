# $NetBSD: Makefile,v 1.2 2018/06/14 08:22:52 yamaguchi Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_vlan

.for name in vlan
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

PROGS=			siocXmulti
MAN.siocXmulti=		#empty
BINDIR.siocXmulti=	${TESTSDIR}

.include <bsd.test.mk>
