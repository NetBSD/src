#	$NetBSD: Makefile,v 1.20 2003/12/11 09:46:27 dyoung Exp $

.include <bsd.own.mk>

PROG=	sshd
MAN=	sshd.8 sshd_config.5 moduli.5

BINDIR=	/usr/sbin

SRCS=	sshd.c auth-rhosts.c auth-passwd.c auth-rsa.c auth-rh-rsa.c \
	sshpty.c sshlogin.c servconf.c serverloop.c uidswap.c \
	auth.c auth1.c auth2.c auth-options.c session.c \
	auth-chall.c auth2-chall.c groupaccess.c \
	auth-skey.c auth-bsdauth.c auth2-hostbased.c auth2-kbdint.c \
	auth2-none.c auth2-passwd.c auth2-pubkey.c \
	monitor_mm.c monitor.c monitor_wrap.c monitor_fdpass.c \
	kexdhs.c kexgexs.c

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKRB5 -I${DESTDIR}/usr/include/krb5
CPPFLAGS+=-I${DESTDIR}/usr/include/kerberosIV
SRCS+=	auth-krb5.c auth2-krb5.c
LDADD+=	-lkrb5 -lasn1
DPADD+=	${LIBKRB5} ${LIBASN1}

.if ${MKKERBEROS4} != "no"
CPPFLAGS+=-DKRB4 -DAFS
SRCS+=	auth-krb4.c
LDADD+=	-lkrb -ldes -lkafs
DPADD+=	${LIBKRB} ${LIBDES} ${LIBKAFS}
.endif

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX

LDADD+=	-lcrypt -lcrypto -lutil -lz
DPADD+=	${LIBCRYPT} ${LIBCRYPTO} ${LIBUTIL} ${LIBZ}

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
LDADD+=	-lskey
DPADD+=	${LIBSKEY}
.endif
