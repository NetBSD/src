#	$NetBSD: Makefile,v 1.1.4.2 2002/12/07 00:35:26 he Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/gnu/dist/mrinfo ${NETBSDSRCDIR}/gnu/dist/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/gnu/dist/mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
