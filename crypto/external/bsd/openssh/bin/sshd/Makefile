#	$NetBSD: Makefile,v 1.27.2.1 2025/08/02 05:18:44 perseant Exp $

.include <bsd.own.mk>

USE_LDAP=	no
PROG=	sshd
MAN=	sshd.8 sshd_config.5 moduli.5

BINDIR=	/usr/sbin

SRCS=	sshd.c sshpty.c srclimit.c servconf.c auth2-methods.c \
	groupaccess.c

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
