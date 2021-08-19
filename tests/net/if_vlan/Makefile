# $NetBSD: Makefile,v 1.4 2021/08/19 03:27:05 yamaguchi Exp $
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

PROGS+=			bpfopen
MAN.bpfopen=	#empty
BINDIR.bpfopen=	${TESTSDIR}
DPADD.bpfopen+=	${LIBUTIL}
LDADD.bpfopen+=	-lutil

.include <bsd.test.mk>
