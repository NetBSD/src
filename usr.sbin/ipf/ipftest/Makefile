#	$NetBSD: Makefile,v 1.20 2002/05/13 06:35:47 martti Exp $

PROG=	ipftest
SRCS=	ipt.c parse.c common.c fil.c ipft_sn.c ipft_ef.c ipft_td.c \
	ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c \
	ip_nat.c ip_proxy.c ip_auth.c ipft_hx.c ip_fil.c \
	natparse.c facpri.c printnat.c printstate.c ip_log.c
MAN=	ipftest.1
CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf -DIPFILTER_LKM -DIPFILTER_LOG

.PATH:	${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
