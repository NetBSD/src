#	$NetBSD: Makefile,v 1.8 2001/06/15 12:51:59 wiz Exp $

.include <bsd.own.mk>

PROG=	sshd
MAN=	sshd.8 primes.5

BINDIR=	/usr/sbin

SRCS=	sshd.c auth-rhosts.c auth-passwd.c auth-rsa.c auth-rh-rsa.c \
	sshpty.c sshlogin.c servconf.c serverloop.c \
	auth.c auth1.c auth2.c auth-options.c session.c \
	auth-chall.c auth2-chall.c groupaccess.c

.if (${MKKERBEROS} != "no")
CPPFLAGS+=-DKRB5 -DAFS -I${DESTDIR}/usr/include/krb5
SRCS+=	auth-krb5.c
LDADD+=	-lkrb5 -lkafs -lasn1
DPADD+=	${LIBKRB5} ${LIBKAFS} ${LIBASN1}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
SRCS+=	auth-krb4.c
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lcrypt -lcrypto -lutil -lz
DPADD+=	${LIBCRYPT} ${LIBCRYPTO} ${LIBUTIL} ${LIBZ}

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.ifdef SKEY
CPPFLAGS+=-DSKEY
LDADD+=	-lskey
DPADD+=	${LIBSKEY}
.endif
