#	$NetBSD: Makefile.inc,v 1.9 2005/02/13 18:15:05 christos Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP
.PATH: ${SSHDIST}

LDADD+=	-lssh -lcrypto -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBZ}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
