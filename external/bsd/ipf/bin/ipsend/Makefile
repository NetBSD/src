#	$NetBSD: Makefile,v 1.3 2013/01/02 15:50:34 dsl Exp $

.include <bsd.own.mk>
.include "../Makefile.inc"

PROG=		ipsend
SRCS=		ipsend.c ip.c ipsopt.c iplang_y.c iplang_l.l sbpf.c \
		sock.c 44arp.c
MAN=		ipsend.1 ipsend.5
DPADD+=		${LIBL}
LDADD+=		-ll

CPPFLAGS+=	-I${UDIST}/ipsend
CPPFLAGS+=	-I${UDIST}/iplang
CPPFLAGS+=	-I${.CURDIR} -I.

# There is some very dubious code to locate the values of
# tcb.rcv_adv, tcb.snd_nxt and tcb.rcv_nxt for a socket.
CPPFLAGS+=	-D_KMEMUSER

CLEANFILES+=	iplang_y.c iplang_y.h

DPSRCS+=	iplang_y.h

.PATH:		${UDIST}/ipsend \
		${UDIST}/iplang

iplang_y.c: iplang_y.y
	${_MKTARGET_CREATE}
	${YACC} -d ${.ALLSRC}
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
