#	$NetBSD: Makefile,v 1.9.2.1 1997/10/30 07:17:09 mrg Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c iplang_l.l iplang_y.y
CPPFLAGS+=-I${.CURDIR}/../ipftest -I${.CURDIR}/../ipf -I.
LDADD=	-ll
DPADD=	${LIBL}

CLEANFILES+= y.tab.h

MAN=	ipsend.1 ipsend.5

.PATH: ${.CURDIR}/../ipf
.include <bsd.prog.mk>
