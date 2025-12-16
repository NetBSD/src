#	$NetBSD: Makefile.inc,v 1.18 2025/12/16 04:32:03 riastradh Exp $

WARNS?=	4

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server
WITH_OPENSSL=1

SSHDIST?= ${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist

CPPFLAGS+=-I${SSHDIST}

CPPFLAGS+=-D_OPENBSD_SOURCE 
CPPFLAGS+=-DHAVE_DLOPEN
CPPFLAGS+=-DHAVE_HEADER_AD
CPPFLAGS+=-DHAVE_LOGIN_CAP
CPPFLAGS+=-DHAVE_STDLIB_H
.if ${HAVE_OPENSSL} < 11
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L
.endif

CPPFLAGS+=-DWITH_OPENSSL -DENABLE_PKCS11 -DWITH_XMSS -DWITH_ZLIB -DWITH_DSA
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

CPPFLAGS+=-DX11BASE=\"/usr/X11R7\"

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX
CPPFLAGS+=-DLIBWRAP

LINTFLAGS+=	-X 117	# nonportable '>>' on signed integer type
LINTFLAGS+=	-X 231	# parameter unused
LINTFLAGS+=	-X 247	# pointer cast to unrelated type
LINTFLAGS+=	-X 351	# missing header declaration

# We install libssh in /usr/lib/private so applications don't
# accidentally link against it.  Set LIBSUBDIR so libssh.so.N and
# libssh.so.N.M will be installed there there, and set PROGDPSUBDIRS so
# ssh/sshd/sshd-session/&c. will find it at link-time.
LIBSUBDIR=	private
PROGDPSUBDIRS+=	private
