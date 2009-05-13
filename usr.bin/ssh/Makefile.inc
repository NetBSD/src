#	$NetBSD: Makefile.inc,v 1.18.2.1 2009/05/13 19:20:06 jym Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP -DHAVE_MMAP -DHAVE_OPENPTY
.PATH: ${SSHDIST}

LDADD+=	-lssh -lcrypto -lcrypt -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBCRYPT} ${LIBZ}

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
.else	# USE_PAM == no
.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
.endif
.endif	# USE_PAM == no

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DGSSAPI -I${DESTDIR}/usr/include/gssapi
CPPFLAGS+=-DKRB5 -I${DESTDIR}/usr/include/krb5 -DHEIMDAL
.endif

.if ${X11FLAVOUR} == "Xorg"
CPPFLAGS+=-DX11BASE=\"/usr/X11R7\"
.endif

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX
CPPFLAGS+=-DLIBWRAP

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
