#	$NetBSD: Makefile,v 1.15 2002/09/18 03:54:31 lukem Exp $

.include <bsd.own.mk>

PROG=	ipsend
SRCS=	ipsend.c ip.c ipsopt.c iplang_l.l iplang_y.y sbpf.c sock.c 44arp.c
YHEADER=1
LDADD=	-ll
DPADD=	${LIBL}

CPPFLAGS+= -I.  -I${NETBSDSRCDIR}/dist/ipf \
		-I${NETBSDSRCDIR}/dist/ipf/ipsend \
		-I${NETBSDSRCDIR}/dist/ipf/iplang

MAN=	ipsend.1 ipsend.5

.PATH:	${NETBSDSRCDIR}/dist/ipf/ipsend \
	${NETBSDSRCDIR}/dist/ipf/iplang

# XXX
# We have a problem with make and linking ipsend
# cc   -o /home/source/src/usr.sbin/ipf/ipsend/../../../dist/ipf/ipsend .....
# isn't correct.
# Use .NOPATH as an workaround for that problem
.NOPATH: ipsend

.include <bsd.prog.mk>
