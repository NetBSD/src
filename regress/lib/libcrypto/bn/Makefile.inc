#	$NetBSD: Makefile.inc,v 1.7 2005/03/09 03:11:21 christos Exp $

.include <bsd.own.mk>
CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.PATH:	${OPENSSLSRC}/crypto/bn

CPPFLAGS+=	-I${OPENSSLSRC}
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
LDADD+=		-lcrypto -lcrypt

# do not install regression test programs
proginstall::
