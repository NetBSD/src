#	$NetBSD: Makefile.inc,v 1.10 2005/02/13 19:15:43 christos Exp $

.include <bsd.own.mk>

SSHDIST?= ${NETBSDSRCDIR}/crypto/dist/ssh

CPPFLAGS+=-I${SSHDIST} -DHAVE_LOGIN_CAP -DHAVE_MMAP -DHAVE_OPENPTY
.PATH: ${SSHDIST}

LDADD+=	-lssh -lcrypto -lz
DPADD+=	${LIBSSH} ${LIBCRYPTO} ${LIBZ}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
