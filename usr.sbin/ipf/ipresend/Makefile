#	$NetBSD: Makefile,v 1.16 2004/03/28 09:01:28 martti Exp $

.include <bsd.own.mk>

PROG=		ipresend
SRCS=		ipresend.c ip.c resend.c sbpf.c sock.c 44arp.c
MAN=		ipresend.1
LDADD+=		-lipf

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf

.PATH:		${NETBSDSRCDIR}/dist/ipf/ipsend

.include <bsd.prog.mk>
