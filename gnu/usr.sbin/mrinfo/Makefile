#	$NetBSD: Makefile,v 1.1.2.2 2002/10/01 04:15:00 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.PATH: ${.CURDIR}/../../dist/mrinfo ${.CURDIR}/../../dist/mrouted
CPPFLAGS+= -I${.CURDIR}/../../dist/mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
