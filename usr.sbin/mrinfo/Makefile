#	$NetBSD: Makefile,v 1.7 2002/10/01 03:40:32 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/gnu/dist/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/gnu/dist/mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
