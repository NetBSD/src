#	$NetBSD: Makefile,v 1.12 1999/12/12 11:30:19 veego Exp $

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c 44arp.c sbpf.c iplang_l.l iplang_y.y
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
