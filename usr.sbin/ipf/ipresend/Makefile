#	$NetBSD: Makefile,v 1.1.1.6 1997/10/30 05:29:46 mrg Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c
MAN=	ipresend.1
CPPFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat ${.CURDIR}/../ipsend ${.CURDIR}/../ipftest
.include <bsd.prog.mk>
