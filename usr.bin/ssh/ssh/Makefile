#	$NetBSD: Makefile,v 1.14 2003/07/23 03:52:25 itojun Exp $

.include <bsd.own.mk>

PROG=	ssh

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKRB5 -I${DESTDIR}/usr/include/krb5
LDADD+=	-lkrb5 -lkafs -lasn1 -lcom_err -lroken
DPADD+=	${LIBKRB5} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lz -lcrypto
DPADD+=	${LIBZ} ${LIBCRYPTO}
