# $NetBSD: Makefile,v 1.1.4.2 2013/06/23 06:26:27 tls Exp $

.include <bsd.own.mk>

SRCS   = omshell.c
PROG   = omshell
MAN    = omshell.1

DHCPSRCDIR=	dhcpctl

CTLOBJDIR!=cd $(.CURDIR)/../../lib/dhcpctl && ${PRINTOBJDIR}

LDADD+=	${CTLOBJDIR}/libdhcpctl.a
DPADD+=	${CTLOBJDIR}/libdhcpctl.a

.include <bsd.prog.mk>
