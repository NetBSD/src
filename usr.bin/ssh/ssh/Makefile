#	$NetBSD: Makefile,v 1.22 2005/08/18 00:19:28 christos Exp $

.include <bsd.own.mk>

PROG=	ssh
SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")

# this is not entirely true, libgssapi might be independant of krb5
SRCS +=	gss-genr.c
LDADD+=	-lgssapi
DPADD+=	${LIBGSSAPI}

LDADD+=	-lkrb5 -lkafs -lasn1 -lcrypt
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1} ${LIBCRYPT}

.if ${MKKERBEROS4} != "no"
SRCS+=radix.c
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>
