#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 15:12:01 lukem Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../mrouted
CPPFLAGS+= -I${.CURDIR}/../mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
