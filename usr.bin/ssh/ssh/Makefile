#	$NetBSD: Makefile,v 1.10 2002/03/22 18:10:23 thorpej Exp $

.include <bsd.own.mk>

PROG=	ssh

BINOWN=	root

BINMODE=0555

LINKS=	${BINDIR}/ssh ${BINDIR}/slogin
MLINKS=	ssh.1 slogin.1

SRCS=	ssh.c readconf.c clientloop.c sshtty.c \
	sshconnect.c sshconnect1.c sshconnect2.c

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKRB5 -DAFS -I${DESTDIR}/usr/include/krb5
LDADD+=	-lkrb5 -lkafs -lasn1
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lz -lcrypto
DPADD+=	${LIBZ} ${LIBCRYPTO}
