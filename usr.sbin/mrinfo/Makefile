#	$NetBSD: Makefile,v 1.9 2003/03/05 21:58:20 wiz Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

.PATH: ${NETBSDSRCDIR}/usr.sbin/mrouted
CPPFLAGS+= -I${NETBSDSRCDIR}/usr.sbin/mrouted

PROG=	mrinfo
SRCS=	igmp.c inet.c kern.c mrinfo.c
MAN=	mrinfo.8
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
