#	$NetBSD: Makefile,v 1.1.1.8 1997/11/14 08:03:33 mrg Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c ip_fil.c fil.c parse.c ip_proxy.c ip_auth.c
MAN=	ipftest.1
CPPFLAGS+=-I${.CURDIR} -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
