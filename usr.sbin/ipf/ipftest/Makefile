#	$NetBSD: Makefile,v 1.1.1.9 1998/05/17 16:29:46 veego Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c ip_fil.c fil.c parse.c ip_proxy.c ip_auth.c
MAN=	ipftest.1
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
