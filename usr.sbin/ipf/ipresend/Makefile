#	$NetBSD: Makefile,v 1.11 1997/10/25 06:58:15 lukem Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c
MAN=	ipresend.1
CPPFLAGS+=-I${.CURDIR}/../ipf -I${.CURDIR}/../ipfstat
CPPFLAGS+=-I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat
.PATH: ${.CURDIR}/../ipsend ${.CURDIR}/../ipftest
.include <bsd.prog.mk>
