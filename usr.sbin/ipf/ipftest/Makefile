#	$NetBSD: Makefile,v 1.1.1.1 1997/01/05 13:09:04 mrg Exp $

PROG=	ipftest
SRCS=	parse.c fil.c ipt.c ipft_sn.c ipft_ef.c ipft_td.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../../../sys/netinet
.include <bsd.prog.mk>
