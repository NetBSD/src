#	$NetBSD: Makefile,v 1.12.2.1 1999/12/20 21:07:01 he Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c ipft_pc.c
MAN=	ipresend.1

CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend
.include <bsd.prog.mk>
