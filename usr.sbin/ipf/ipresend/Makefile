#	$NetBSD: Makefile,v 1.14 2000/05/03 11:40:56 veego Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c ipft_ef.c ipft_hx.c ipft_pc.c ipft_sn.c \
	ipft_td.c ipft_tx.c opt.c sbpf.c sock.c 44arp.c
MAN=	ipresend.1

CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend
.include <bsd.prog.mk>
