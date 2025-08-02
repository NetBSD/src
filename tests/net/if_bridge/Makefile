# $NetBSD: Makefile,v 1.3.24.1 2025/08/02 05:58:10 perseant Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if_bridge
FILESDIR=	${TESTSDIR}

.for name in bridge rtable
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

FILES+=		broadcast.pcap.uue unicast.pcap.uue

.include <bsd.test.mk>
