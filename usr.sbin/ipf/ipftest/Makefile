#	$NetBSD: Makefile,v 1.17.2.1 2002/02/09 16:56:45 he Exp $

PROG=	ipftest
SRCS=	ipt.c parse.c common.c fil.c ipft_sn.c ipft_ef.c ipft_td.c \
	ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c \
	ip_nat.c ip_proxy.c ip_auth.c ipft_hx.c ip_fil.c \
	natparse.c facpri.c printnat.c printstate.c
MAN=	ipftest.1
CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

SUBDIR=	test

.PATH:	${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
