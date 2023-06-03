#	$NetBSD: Makefile,v 1.27 2023/06/03 09:09:02 lukem Exp $

.include <bsd.own.mk>

PROG=	sshd
MAN=	sshd.8 sshd_config.5 moduli.5

BINDIR=	/usr/sbin

SRCS=	sshd.c auth-rhosts.c auth-passwd.c auth2-pubkeyfile.c \
	sshpty.c sshlogin.c servconf.c serverloop.c \
	auth.c auth2.c auth-options.c session.c \
	auth-krb5.c auth2-chall.c groupaccess.c \
	auth-bsdauth.c auth2-hostbased.c auth2-kbdint.c \
	auth2-none.c auth2-passwd.c auth2-pubkey.c \
	monitor.c monitor_wrap.c srclimit.c \
	kexgexs.c sftp-server.c sftp-common.c \
	sftp-realpath.c sandbox-rlimit.c pfilter.c

COPTS.auth-options.c+=	-Wno-pointer-sign
COPTS.ldapauth.c+=	-Wno-format-nonliteral	# XXX: should fix

.if (${USE_PAM} != "no")
SRCS+=	auth-pam.c
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
SRCS+=	gss-genr.c auth2-gss.c gss-serv.c gss-serv-krb5.c
LDADD+=	-lgssapi -lheimntlm
DPADD+=	${LIBGSSAPI} ${LIBHEIMNTLM}

LDADD+=	-lkafs
DPADD+= ${LIBKAFS}

SRCS+=	auth2-krb5.c
LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}
.endif

.if (${USE_LDAP} != "no")
SRCS+=	ldapauth.c
LDADD+=	${LIBLDAP_LDADD}
DPADD+=	${LIBLDAP_DPADD}
.endif

LDADD+=	-lcrypt -lutil
DPADD+=	${LIBCRYPT} ${LIBUTIL}

LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.ifdef CRUNCHEDPROG
CPPFLAGS+=-DSMALL
.else
LDADD+=	-lblocklist
DPADD+=	${LIBBLOCKLIST}
.endif

COPTS.sshlogin.c+=	${CC_WNO_STRINGOP_TRUNCATION}
COPTS.ldapauth.c+=	${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_STRINGOP_OVERFLOW}
COPTS.monitor.c+= -Wno-error=deprecated-declarations
COPTS.kexgexs.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
