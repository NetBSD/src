#	$NetBSD: Makefile,v 1.5 1997/04/13 14:51:54 mrg Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat ${.CURDIR}/../ipsend ${.CURDIR}/../ipftest
.include <bsd.prog.mk>
