#	$NetBSD: Makefile.inc,v 1.16 2008/04/06 09:54:37 lukem Exp $

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

CPPFLAGS+=-DSUPPORT_UTMP -DSUPPORT_UTMPX
CPPFLAGS+=-DLIBWRAP

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
