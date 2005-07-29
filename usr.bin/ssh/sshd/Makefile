#	$NetBSD: Makefile,v 1.26 2005/07/29 01:19:20 christos Exp $

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
	monitor_mm.c monitor.c monitor_wrap.c \
	kexdhs.c kexgexs.c

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
SRCS+= auth-pam.c
LDADD+=	-lpam ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}

.if ${USE_YP} != "no"
LDADD+= -lrpcsvc
DPADD+= ${LIBRPCSVC}
.endif

.endif

.if (${USE_KERBEROS} != "no")

CPPFLAGS+=-DGSSAPI -I${DESTDIR}/usr/include/gssapi
SRCS +=	gss-genr.c auth2-gss.c gss-serv.c gss-serv-krb5.c
LDADD+=	-lgssapi
DPADD+=	${LIBGSSAPI}

LDADD+=	-lkafs
DPADD+= ${LIBKAFS}

CPPFLAGS+=-DKRB5 -I${DESTDIR}/usr/include/krb5
SRCS+=	auth-krb5.c auth2-krb5.c
LDADD+=	-lkrb5 -lasn1
DPADD+=	${LIBKRB5} ${LIBASN1}

.if ${MKKERBEROS4} != "no"
CPPFLAGS+=-DKRB4 -DAFS -I${DESTDIR}/usr/include/kerberosIV
SRCS+=	auth-krb4.c radix.c
LDADD+=	-lkrb -ldes
DPADD+=	${LIBKRB} ${LIBDES}
.endif

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX

LDADD+=	-lcrypt -lutil
DPADD+=	${LIBCRYPT} ${LIBUTIL}

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
LDADD+=	-lskey
DPADD+=	${LIBSKEY}
.endif
