# $NetBSD: Makefile,v 1.1.10.2 2014/05/22 15:44:30 yamt Exp $

.include <bsd.own.mk>

SRCS   = omshell.c
PROG   = omshell
MAN    = omshell.1

DHCPSRCDIR=	dhcpctl

CTLOBJDIR!=cd $(.CURDIR)/../../lib/dhcpctl && ${PRINTOBJDIR}

LDADD+=	${CTLOBJDIR}/libdhcpctl.a
DPADD+=	${CTLOBJDIR}/libdhcpctl.a

.include <bsd.prog.mk>
