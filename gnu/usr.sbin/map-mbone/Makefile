#	$NetBSD: Makefile,v 1.1.2.2 2002/10/01 04:15:01 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../../dist/map-mbone ${.CURDIR}/../../dist/mrouted
CPPFLAGS+= -I${.CURDIR}/../../dist/mrouted

PROG=	map-mbone
SRCS=	igmp.c inet.c kern.c mapper.c
MAN=	map-mbone.8

.include <bsd.prog.mk>
