#	$NetBSD: Makefile,v 1.11.4.1 2017/12/04 10:55:18 snj Exp $

.include <bsd.own.mk>

BINDIR = /usr/bin

PROG=	ssh
SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect2.c mux.c auth.c

COPTS.auth.c=	-DHOST_ONLY
COPTS.mux.c=		-Wno-pointer-sign
COPTS.sshconnect2.c=	-Wno-pointer-sign

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")
# this is not entirely true, libgssapi might be independent of krb5
SRCS +=	gss-genr.c
LDADD+=	-lgssapi -lheimntlm
DPADD+=	${LIBGSSAPI} ${LIBHEIMNTLM}

LDADD+=	-lkrb5 -lhx509 -lheimbase -lwind
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBHEIMBASE} ${LIBWIND}

LDADD+=	-lkafs -lasn1 -lcrypt
DPADD+=	${LIBKAFS} ${LIBASN1} ${LIBCRYPT}

LDADD+=	-lcom_err -lroken -lsqlite3 -lutil
DPADD+=	${LIBCOM_ERR} ${LIBROKEN} ${LIBSQLITE3} ${LIBUTIL}
.endif

.include <bsd.prog.mk>
