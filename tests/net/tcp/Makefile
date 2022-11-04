# $NetBSD: Makefile,v 1.2 2022/11/04 08:01:42 ozaki-r Exp $
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

.include <bsd.test.mk>
