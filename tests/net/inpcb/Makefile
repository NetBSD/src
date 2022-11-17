# $NetBSD: Makefile,v 1.1 2022/11/17 08:45:35 ozaki-r Exp $
#

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/net/inpcb

.for name in inpcb_bind broadcast_bind
TESTS_SH+=		t_${name}
TESTS_SH_SRC_t_${name}=	../net_common.sh t_${name}.sh
.endfor

PROGS=			inpcb_bind
MAN.inpcb_bind=	# empty
BINDIR.inpcb_bind=	${TESTSDIR}

PROGS+=			broadcast_bind
MAN.broadcast_bind=	# empty
BINDIR.broadcast_bind=	${TESTSDIR}

.include <bsd.test.mk>
