#	$NetBSD: Makefile,v 1.1.1.5 1997/07/05 05:13:03 darrenr Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c ip_fil.c fil.c parse.c ip_proxy.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
