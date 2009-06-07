#	$NetBSD: Makefile,v 1.1 2009/06/07 22:38:46 christos Exp $

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
	kexdhs.c kexgexs.c sftp-server.c sftp-common.c

.if (${USE_PAM} != "no")
SRCS+= auth-pam.c
LDADD+=	-lpam ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}

.if ${USE_YP} != "no"
LDADD+= -lrpcsvc
DPADD+= ${LIBRPCSVC}
.endif

.else	# USE_PAM == no

.if (${USE_SKEY} != "no")
LDADD+=	-lskey
DPADD+=	${LIBSKEY}
.endif

.endif	# USE_PAM == no

.if (${USE_KERBEROS} != "no")

SRCS +=	gss-genr.c auth2-gss.c gss-serv.c gss-serv-krb5.c
LDADD+=	-lgssapi -lheimntlm
DPADD+=	${LIBGSSAPI} ${LIBHEIMNTLM}

LDADD+=	-lkafs
DPADD+= ${LIBKAFS}

SRCS+=	auth-krb5.c auth2-krb5.c
LDADD+=	-lkrb5 -lasn1
DPADD+=	${LIBKRB5} ${LIBASN1}

LDADD+=	-lcom_err -lroken
DPADD+=	${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>

LDADD+=	-lcrypt -lutil
DPADD+=	${LIBCRYPT} ${LIBUTIL}

LDADD+=	-lwrap
DPADD+=	${LIBWRAP}
