#	$NetBSD: Makefile,v 1.19 2005/01/03 06:05:50 lukem Exp $

.include <bsd.own.mk>

PROG=	ssh
SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MAN=	ssh.1 ssh_config.5
MLINKS=	ssh.1 slogin.1

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-I${DESTDIR}/usr/include/kerberosIV
CPPFLAGS+=-DKRB5 -DAFS -I${DESTDIR}/usr/include/krb5
LDADD+=	-lkrb5 -lkafs -lasn1
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1}

.if ${MKKERBEROS4} != "no"
CPPFLAGS+=-DKRB4
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>
