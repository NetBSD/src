#	$NetBSD: Makefile,v 1.3 2000/10/17 23:51:24 tv Exp $

.include <bsd.own.mk>

PROG=	ssh

BINOWN=	root
BINMODE=0555

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
