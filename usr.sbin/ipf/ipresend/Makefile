#	$NetBSD: Makefile,v 1.4 1997/03/29 20:15:45 thorpej Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../../../sbin/ipf

.PATH: ${.CURDIR}/../../../sbin/ipf ${.CURDIR}/../ipfstat ${.CURDIR}/../ipsend ${.CURDIR}/../ipftest
.include <bsd.prog.mk>
