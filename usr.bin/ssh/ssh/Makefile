#	$NetBSD: Makefile,v 1.6 2001/03/04 00:41:28 assar Exp $

.include <bsd.own.mk>

PROG=	ssh

BINOWN=	root
BINMODE=0555

SRCS=	ssh.c log-client.c readconf.c clientloop.c \
	sshconnect.c sshconnect1.c sshconnect2.c

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MLINKS=	ssh.1 slogin.1

.if (${MKKERBEROS} != "no")
CPPFLAGS+=-DKRB5 -DAFS -I${DESTDIR}/usr/include/krb5
LDADD+=	-lkrb5 -lkafs -lasn1
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lutil -lz -lcrypto
DPADD+=	${LIBUTIL} ${LIBZ} ${LIBCRYPTO}
