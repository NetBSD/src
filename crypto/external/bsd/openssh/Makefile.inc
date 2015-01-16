#	$NetBSD: Makefile.inc,v 1.7 2015/01/16 20:17:49 christos Exp $

WARNS?=	4

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server
WITH_OPENSSL=1

SSHDIST?= ${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP -DHAVE_MMAP -DHAVE_OPENPTY
CPPFLAGS+=-DWITH_SSH1 -DWITH_OPENSSL -DENABLE_PKCS11 -D_OPENBSD_SOURCE
.if !defined(NOPIC)
CPPFLAGS+=-DHAVE_DLOPEN
.endif
.PATH: ${SSHDIST}

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
.else	# USE_PAM == no
.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
.endif
.endif	# USE_PAM == no

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DGSSAPI
CPPFLAGS+=-DKRB5 -DHEIMDAL
.endif

.if (${USE_LDAP} != "no")
CPPFLAGS+=-DWITH_LDAP_PUBKEY
.endif

.if ${X11FLAVOUR} == "Xorg"
CPPFLAGS+=-DX11BASE=\"/usr/X11R7\"
.endif

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX
CPPFLAGS+=-DLIBWRAP
