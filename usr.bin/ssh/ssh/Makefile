#	$NetBSD: Makefile,v 1.1.2.2 2000/10/18 04:17:34 tv Exp $

.include <bsd.own.mk>

PROG=	ssh

BINOWN=	root
BINMODE=4555

SRCS=	ssh.c log-client.c readconf.c clientloop.c sshconnect.c \
	sshconnect1.c sshconnect2.c

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MLINKS=	ssh.1 slogin.1

.if (${MKKERBEROS} != "no")
# XXX Needs krb5
#CPPFLAGS+=-DAFS
#LDADD+=	-lkafs
#DPADD+=	${LIBKAFS}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lutil -lz
DPADD+=	${LIBCRYPTO} ${LIBUTIL} ${LIBZ}
