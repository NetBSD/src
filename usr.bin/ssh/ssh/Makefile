#	$NetBSD: Makefile,v 1.4 2001/01/14 05:22:33 itojun Exp $

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
#LDADD+=	-lkrb5 -lkafs -lasn1
#DPADD+=	${LIBKAFS}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lutil -lz
DPADD+=	${LIBCRYPTO} ${LIBUTIL} ${LIBZ}
