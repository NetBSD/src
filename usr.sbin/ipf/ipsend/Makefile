#	$NetBSD: Makefile,v 1.13 2000/05/03 11:40:56 veego Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c iplang_l.l iplang_y.y sbpf.c sock.c 44arp.c
YHEADER=1
LDADD=	-ll
DPADD=	${LIBL}

CPPFLAGS+=-I${.CURDIR}/../../../dist/ipf -I${.CURDIR}/../../../dist/ipf/ipsend -I.

MAN=	ipsend.1 ipsend.5

.PATH:	${.CURDIR}/../../../dist/ipf/ipsend \
	${.CURDIR}/../../../dist/ipf/iplang

# XXX
# We have a problem with make and linking ipsend
# cc   -o /home/source/src/usr.sbin/ipf/ipsend/../../../dist/ipf/ipsend .....
# isn't correct.
# Use .NOPATH as an workaround for that problem
.NOPATH: ipsend

.include <bsd.prog.mk>
