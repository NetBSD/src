#	$NetBSD: Makefile,v 1.20 2004/03/29 04:40:06 lukem Exp $

NOGCCERROR=	# defined

.include <bsd.own.mk>

PROG=		ipsend
SRCS=		ipsend.c ip.c ipsopt.c iplang_y.c iplang_l.c sbpf.c \
		sock.c 44arp.c
MAN=		ipsend.1 ipsend.5
DPADD+=		${LIBL}
LDADD+=		-ll

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/ipsend
CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/iplang

CLEANFILES+=	iplang_y.c iplang_y.h
CLEANFILES+=	iplang_l.c

.PATH:		${NETBSDSRCDIR}/dist/ipf/ipsend \
		${NETBSDSRCDIR}/dist/ipf/iplang

iplang_y.c: iplang_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${NETBSDSRCDIR}/dist/ipf/iplang/iplang_y.y
	mv y.tab.c ${.TARGET}
	mv y.tab.h ${.TARGET:.c=.h}

iplang_y.h: iplang_y.c

iplang_l.l: iplang_y.h

# XXX
# We have a problem with make and linking ipsend
# cc   -o /home/source/src/usr.sbin/ipf/ipsend/../../../dist/ipf/ipsend .....
# isn't correct.
# Use .NOPATH as an workaround for that problem
.NOPATH: ipsend

.include <bsd.prog.mk>
