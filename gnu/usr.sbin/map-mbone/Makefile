#	$NetBSD: Makefile,v 1.1 2002/10/01 03:50:12 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/gnu/dist/map-mbone ${NETBSDSRCDIR}/gnu/dist/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/gnu/dist/mrouted

PROG=	map-mbone
SRCS=	igmp.c inet.c kern.c mapper.c
MAN=	map-mbone.8

.include <bsd.prog.mk>
