#	$NetBSD: Makefile,v 1.3 2013/01/02 18:41:49 dsl Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		iptest
SRCS=		iptest.c iptests.c ip.c sbpf.c sock.c 44arp.c
MAN=		iptest.1

CPPFLAGS+=	-I${.CURDIR}/../ipsend -I${NETBSDSRCDIR}/sys
.PATH:		${UDIST}/ipsend

# There is some very dubious code to locate the values of
# tcb.rcv_adv, tcb.snd_nxt and tcb.rcv_nxt for a socket.
CPPFLAGS+=      -D_KMEMUSER

.include <bsd.prog.mk>
