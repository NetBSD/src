#	$NetBSD: Makefile,v 1.1.1.2 1997/03/29 02:49:55 darrenr Exp $

PROG=	ipftest
SRCS=	ipt.c ipft_sn.c ipft_ef.c ipft_td.c ipft_pc.c opt.c ipft_tx.c misc.c ip_frag.c ip_state.c ip_nat.c ipft_hx.c
NOMAN=	yes
CFLAGS+=	-I${.CURDIR}/../../sbin/ipf

.PATH: ${.CURDIR}/../../sbin/ipf
.include <bsd.prog.mk>
