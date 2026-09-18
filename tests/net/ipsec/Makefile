# $NetBSD: Makefile,v 1.13 2026/09/18 13:32:55 riastradh Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/ipsec

.for name in ipsec_ah_keys ipsec_esp_keys ipsec_forwarding ipsec_gif \
    ipsec_l2tp ipsec_misc ipsec_natt ipsec_sockopt ipsec_spflags ipsec_sysctl \
    ipsec_tcp ipsec_transport ipsec_tunnel ipsec_tunnel_ipcomp ipsec_tunnel_odd
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh ./common.sh ./algorithms.sh \
    t_${name}.sh
.endfor

TESTS_C+=		t_ipsec_policy

PROGS=			natt_terminator
MAN.natt_terminator=	# empty
BINDIR.natt_terminator=	${TESTSDIR}

.include <bsd.test.mk>
