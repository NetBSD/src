#	$NetBSD: Makefile,v 1.19.2.4 2004/03/31 07:59:48 tron Exp $

NOGCCERROR=	# defined

.include <bsd.own.mk>

PROG=		ipsend
SRCS=		ipsend.c ip.c ipsopt.c iplang_y.c iplang_l.l sbpf.c \
		sock.c 44arp.c
MAN=		ipsend.1 ipsend.5
DPADD+=		${LIBL}
LDADD+=		-ll

CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/ipsend
CPPFLAGS+=	-I${NETBSDSRCDIR}/dist/ipf/iplang
CPPFLAGS+=	-I.

CLEANFILES+=	iplang_y.c iplang_y.h

DPSRCS+=	iplang_y.h

.PATH:		${NETBSDSRCDIR}/dist/ipf/ipsend \
		${NETBSDSRCDIR}/dist/ipf/iplang

iplang_y.c: iplang_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${NETBSDSRCDIR}/dist/ipf/iplang/iplang_y.y
	mv y.tab.c ${.TARGET}
	mv y.tab.h ${.TARGET:.c=.h}

iplang_y.h: iplang_y.c

# XXX
# We have a problem with make and linking ipsend
# cc   -o /home/source/src/usr.sbin/ipf/ipsend/../../../dist/ipf/ipsend .....
# isn't correct.
# Use .NOPATH as an workaround for that problem
.NOPATH: ipsend

.include <bsd.prog.mk>
