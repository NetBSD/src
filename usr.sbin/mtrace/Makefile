#	$NetBSD: Makefile,v 1.7 2002/10/01 03:40:31 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/gnu/dist/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/gnu/dist/mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

BINMODE=4555
BINOWN= root

.include <bsd.prog.mk>
