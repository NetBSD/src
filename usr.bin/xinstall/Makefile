#	$NetBSD: Makefile,v 1.19 2003/05/18 07:57:37 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	xinstall
SRCS=	${HOSTPROG:D:Ustat_flags.c} xinstall.c getid.c
MAN=	install.1

.PATH:		${NETBSDSRCDIR}/bin/ls ${NETBSDSRCDIR}/usr.sbin/mtree
CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls -I${NETBSDSRCDIR}/usr.sbin/mtree

PROGNAME=install

.include <bsd.prog.mk>
