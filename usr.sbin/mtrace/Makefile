#	$NetBSD: Makefile,v 1.6 1997/10/25 06:58:24 lukem Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../mrouted
CPPFLAGS+= -I${.CURDIR}/../mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

BINMODE=4555
BINOWN= root

.include <bsd.prog.mk>
