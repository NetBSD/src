# $NetBSD: Makefile,v 1.7.12.2 2020/04/08 14:09:11 martin Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/if

TESTS_C=	t_compat
.for name in ifconf ifconfig
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

PROGS=		ifconf
MAN.ifconf=	# empty
BINDIR.ifconf=	${TESTSDIR}

LDADD.t_compat=		-lrumpnet_shmif -lrumpnet_netinet -lrumpnet_net -lrumpnet
LDADD.t_compat+=	${LIBRUMPBASE}

.include <bsd.test.mk>
