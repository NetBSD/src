#	$NetBSD: Makefile,v 1.1.2.2 2002/10/01 04:15:02 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../../dist/mtrace ${.CURDIR}/../../dist/mrouted
CPPFLAGS+= -I${.CURDIR}/../../dist/mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

BINMODE=4555
BINOWN= root

.include <bsd.prog.mk>
