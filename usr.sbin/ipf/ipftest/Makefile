#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:33 darrenr Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c ip_fil.c fil.c parse.c
NOMAN=	yes
CFLAGS+=	-I. -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
