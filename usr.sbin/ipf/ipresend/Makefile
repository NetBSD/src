#	$NetBSD: Makefile,v 1.6 1997/05/06 20:46:11 gwr Exp $

PROG=	ipresend
SRCS=	ipresend.c ip.c resend.c opt.c ipft_ef.c ipft_hx.c ipft_sn.c ipft_td.c ipft_tx.c sbpf.c 44arp.c
NOMAN=	yes
CFLAGS+=-I${.CURDIR}/../ipfstat -I${.CURDIR}/../ipsend -I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf

.PATH.c: ${.CURDIR}/../ipf ${.CURDIR}/../ipfstat ${.CURDIR}/../ipsend ${.CURDIR}/../ipftest
.include <bsd.prog.mk>
