#	$NetBSD: Makefile,v 1.27 2006/11/24 22:04:25 wiz Exp $

.include <bsd.own.mk>

PROG=	ssh
SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c

COPTS.sshconnect1.c=	-fno-strict-aliasing
.if ${HAVE_GCC} == 4
COPTS.sshconnect2.c=	-Wno-pointer-sign
.endif

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")

# this is not entirely true, libgssapi might be independent of krb5
SRCS +=	gss-genr.c
LDADD+=	-lgssapi
DPADD+=	${LIBGSSAPI}

LDADD+=	-lkrb5 -lkafs -lasn1 -lcrypt
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1} ${LIBCRYPT}

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>
