#	$NetBSD: Makefile.inc,v 1.16.4.1 2026/01/22 19:51:55 martin Exp $

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
Index: Makefile.inc
===================================================================
RCS file: /cvsroot/src/crypto/external/bsd/openssh/Makefile.inc,v
retrieving revision 1.17
retrieving revision 1.18
diff -c -u -r1.17 -r1.18
--- Makefile.inc	23 Sep 2025 20:18:10 -0000	1.17
+++ Makefile.inc	16 Dec 2025 04:32:03 -0000	1.18
@@ -52,3 +52,10 @@
 LINTFLAGS+=	-X 231	# parameter unused
 LINTFLAGS+=	-X 247	# pointer cast to unrelated type
 LINTFLAGS+=	-X 351	# missing header declaration

# We install libssh in /usr/lib/private so applications don't
# accidentally link against it.  Set LIBSUBDIR so libssh.so.N and
# libssh.so.N.M will be installed there there, and set PROGDPSUBDIRS so
# ssh/sshd/sshd-session/&c. will find it at link-time.
LIBSUBDIR=	private
PROGDPSUBDIRS+=	private
