# $NetBSD: Makefile,v 1.3 2011/06/22 02:49:45 mrg Exp $

.include <bsd.own.mk>

SRCS   = omshell.c
PROG   = omshell
MAN    = omshell.1

DHCPSRCDIR=	dhcpctl

CTLOBJDIR!=cd $(.CURDIR)/../dhcpctl && ${PRINTOBJDIR}

LDADD+=	${CTLOBJDIR}/libdhcpctl.a
DPADD+=	${CTLOBJDIR}/libdhcpctl.a

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.omshell.c+=	-Wno-error
.endif
