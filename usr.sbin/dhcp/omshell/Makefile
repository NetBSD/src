# Makefile

.include <bsd.own.mk>

SRCS   = omshell.c
PROG   = omshell
MAN    = omshell.1

DHCPSRCDIR=	dhcpctl

CTLOBJDIR!=cd $(.CURDIR)/../dhcpctl && ${PRINTOBJDIR}

LDADD+=	${CTLOBJDIR}/libdhcpctl.a
DPADD+=	${CTLOBJDIR}/libdhcpctl.a

.include <bsd.prog.mk>
