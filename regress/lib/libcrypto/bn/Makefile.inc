#	$NetBSD: Makefile.inc,v 1.6 2003/07/24 16:28:59 tron Exp $

.include <bsd.own.mk>
CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.PATH:	${OPENSSLSRC}/crypto/bn

CPPFLAGS+=	-I${OPENSSLSRC}
DPADD+=		${LIBCRYPTO}
LDADD+=		-lcrypto

# do not install regression test programs
proginstall::
