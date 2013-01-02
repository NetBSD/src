#	$NetBSD: Makefile,v 1.3 2013/01/02 15:50:34 dsl Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		ipresend
SRCS=		ipresend.c ip.c resend.c sbpf.c sock.c 44arp.c
MAN=		ipresend.1

CPPFLAGS+=	-I. -I${.CURDIR}/../ipsend -I${NETBSDSRCDIR}/sys 
.PATH:		${UDIST}/ipsend

# There is some very dubious code to locate the values of
# tcb.rcv_adv, tcb.snd_nxt and tcb.rcv_nxt for a socket.
CPPFLAGS+=	-D_KMEMUSER

#.BEGIN:
#	rm -f machine x86
#	ln -s ${NETBSDSRCDIR}/sys/arch/${MACHINE}/include machine
#	ln -s ${NETBSDSRCDIR}/sys/arch/x86/include x86

.include <bsd.prog.mk>
