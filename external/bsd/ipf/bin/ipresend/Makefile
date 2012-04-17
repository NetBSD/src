#	$NetBSD: Makefile,v 1.2.2.2 2012/04/17 00:03:11 yamt Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		ipresend
SRCS=		ipresend.c ip.c resend.c sbpf.c sock.c 44arp.c
MAN=		ipresend.1

CPPFLAGS+=	-I. -I${.CURDIR}/../ipsend -I${NETBSDSRCDIR}/sys 
.PATH:		${UDIST}/ipsend

#.BEGIN:
#	rm -f machine x86
#	ln -s ${NETBSDSRCDIR}/sys/arch/${MACHINE}/include machine
#	ln -s ${NETBSDSRCDIR}/sys/arch/x86/include x86

.include <bsd.prog.mk>
