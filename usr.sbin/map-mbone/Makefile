#	$NetBSD: Makefile,v 1.5 1997/10/25 06:58:20 lukem Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../mrouted
CPPFLAGS+= -I${.CURDIR}/../mrouted

PROG=	map-mbone
SRCS=	igmp.c inet.c kern.c mapper.c
MAN=	map-mbone.8

.include <bsd.prog.mk>
