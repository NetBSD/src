#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp
#	$Id: Makefile,v 1.1 1995/06/01 05:19:10 mycroft Exp $

.PATH: ${.CURDIR}/../mrouted
CFLAGS+= -I${.CURDIR}/../mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8

.include <bsd.prog.mk>
