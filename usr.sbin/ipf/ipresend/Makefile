#	$NetBSD: Makefile,v 1.13 1999/12/12 11:30:17 veego Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c ipft_pc.c
MAN=	ipresend.1

CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend
.include <bsd.prog.mk>
