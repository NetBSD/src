#	$NetBSD: Makefile,v 1.17 2004/03/28 09:21:10 martti Exp $

.include <bsd.own.mk>

PROG=		ipsend
SRCS=		ipsend.c ip.c ipsopt.c iplang_y.c iplang_l.c sbpf.c \
		sock.c 44arp.c
MAN=		ipsend.1 ipsend.5
DPADD+=		${LIBL}
LDADD+=		-ll -lipf

NOGCCERROR=	# defined

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/ipsend
CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/iplang

CLEANFILES+=	iplang_y.c iplang_y.h
CLEANFILES+=	iplang_l.c

.PATH:		${NETBSDSRCDIR}/dist/ipf/ipsend \
		${NETBSDSRCDIR}/dist/ipf/iplang

iplang_y.c iplang_y.h: iplang_y.y
	yacc -d ${NETBSDSRCDIR}/dist/ipf/iplang/iplang_y.y
	mv y.tab.c iplang_y.c
	mv y.tab.h iplang_y.h

# XXX
# We have a problem with make and linking ipsend
# cc   -o /home/source/src/usr.sbin/ipf/ipsend/../../../dist/ipf/ipsend .....
# isn't correct.
# Use .NOPATH as an workaround for that problem
.NOPATH: ipsend

.include <bsd.prog.mk>
