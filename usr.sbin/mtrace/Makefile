#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp
#	$Id: Makefile,v 1.2 1995/06/01 23:38:51 thorpej Exp $

.PATH: ${.CURDIR}/../mrouted
CFLAGS+= -I${.CURDIR}/../mrouted

PROG=	mtrace
SRCS=	igmp.c inet.c kern.c mtrace.c
MAN=	mtrace.8

BINMODE=4555
BINOWN= root

.include <bsd.prog.mk>
