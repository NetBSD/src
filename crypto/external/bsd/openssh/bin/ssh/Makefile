#	$NetBSD: Makefile,v 1.1 2009/06/07 22:38:45 christos Exp $

.include <bsd.own.mk>

BINDIR = /usr/bin

PROG=	ssh
SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c mux.c

COPTS.sshconnect1.c=	-fno-strict-aliasing
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.sshconnect2.c=	-Wno-pointer-sign
.endif

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")

# this is not entirely true, libgssapi might be independent of krb5
SRCS +=	gss-genr.c
LDADD+=	-lgssapi -lheimntlm
DPADD+=	${LIBGSSAPI} ${LIBHEIMNTLM}

LDADD+=	-lkrb5 -lhx509 -lkafs -lasn1 -lcrypt
DPADD+=	${LIBKRB5} ${LIBHX509} ${LIBKAFS} ${LIBASN1} ${LIBCRYPT}

LDADD+=	-lcom_err -lroken -lutil
DPADD+=	${LIBCOM_ERR} ${LIBROKEN} ${LIBUTIL}
.endif

.include <bsd.prog.mk>
