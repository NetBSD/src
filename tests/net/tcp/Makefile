# $NetBSD: Makefile,v 1.3 2022/11/17 08:45:36 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/tcp

.for name in tcp_nc tcp_shutdown
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

PROGS=			tcp_shutdown
MAN.tcp_shutdown=	# empty
BINDIR.tcp_shutdown=	${TESTSDIR}

TESTS_C=	t_tcp_connect_port

LDADD.t_tcp_connect_port+=	-lrumpnet_netinet -lrumpnet_netinet6 -lrumpnet_net
LDADD.t_tcp_connect_port+=	-lrumpnet_local -lrumpnet ${LIBRUMPBASE}

.include <bsd.test.mk>
