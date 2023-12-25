#	$NetBSD: Makefile,v 1.14.4.1 2023/12/25 12:31:01 martin Exp $

.include <bsd.own.mk>

BINDIR = /usr/bin

PROG=	ssh
SRCS=	channels.c ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect2.c mux.c auth.c auth-options.c

COPTS.auth.c=		-DHOST_ONLY
COPTS.mux.c=		-Wno-pointer-sign
COPTS.sshconnect2.c=	-Wno-pointer-sign

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")
# this is not entirely true, libgssapi might be independent of krb5
SRCS +=	gss-genr.c
LDADD+=	-lgssapi -lheimntlm ${LIBKRB5_LDADD}
DPADD+=	${LIBGSSAPI} ${LIBHEIMNTLM} ${LIBKRB5_DPADD}
.endif

COPTS.sshconnect.c+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
