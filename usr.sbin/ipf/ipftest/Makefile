#	$NetBSD: Makefile,v 1.14 1999/12/12 11:30:06 veego Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c ip_fil.c fil.c parse.c facpri.c ip_proxy.c ip_auth.c natparse.c
MAN=	ipftest.1
CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
