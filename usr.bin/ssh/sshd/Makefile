#	$NetBSD: Makefile,v 1.3 2001/02/07 17:05:37 itojun Exp $

.include <bsd.own.mk>

PROG=	sshd
MAN=	sshd.8

BINDIR=	/usr/sbin

SRCS=	sshd.c auth-rhosts.c auth-passwd.c auth-rsa.c auth-rh-rsa.c \
	pty.c log-server.c login.c servconf.c serverloop.c \
	auth.c auth1.c auth2.c auth-options.c session.c dh.c \
	auth-chall.c auth2-chall.c groupaccess.c

.if (${MKKERBEROS} != "no")
# XXX needs krb5
#CPPFLAGS+=-DAFS
#LDADD+=	-lkafs
#DPADD+=	${LIBKAFS}

CPPFLAGS+=-DKRB4 -I${DESTDIR}/usr/include/kerberosIV
SRCS+=	auth-krb4.c
LDADD+=	-lkrb -lcom_err -lroken
DPADD+=	${LIBKRB} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lcrypto -lutil -lz
DPADD+=	${LIBCRYPTO} ${LIBUTIL} ${LIBZ}

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.ifdef SKEY
CPPFLAGS+=-DSKEY
LDADD+=	-lskey
DPADD+=	${LIBSKEY}
.endif
