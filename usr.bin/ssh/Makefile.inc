#	$NetBSD: Makefile.inc,v 1.8 2005/02/13 06:07:54 christos Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP
.if ${USE_PAM} != "no"
CPPFLAGS+=-DUSE_PAM
.endif
.PATH: ${SSHDIST}

.if ${USE_PAM} != "no"
LDADD+= -lpam
DPADD+= ${LIBPAM}
.endif
LDADD+=	-lssh -lcrypto -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBZ}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
