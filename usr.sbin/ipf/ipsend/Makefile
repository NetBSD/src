#	$NetBSD: Makefile,v 1.1.1.10 1998/11/22 14:21:58 mrg Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c iplang_l.l iplang_y.y
CPPFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf -I.
YHEADER=1
LDADD=	-ll
DPADD=	${LIBL}

MAN=	ipsend.1 ipsend.5

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
