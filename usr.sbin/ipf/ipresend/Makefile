#	$NetBSD: Makefile,v 1.15 2002/09/18 03:54:31 lukem Exp $

.include <bsd.own.mk>

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c ipft_ef.c ipft_hx.c ipft_pc.c ipft_sn.c \
	ipft_td.c ipft_tx.c opt.c sbpf.c sock.c 44arp.c
MAN=	ipresend.1

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/ipf

.PATH:	${NETBSDSRCDIR}/dist/ipf/ipsend
.include <bsd.prog.mk>
