#	$NetBSD: Makefile.inc,v 1.5 2003/07/24 14:22:52 itojun Exp $

.include <bsd.own.mk>
CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

.PATH:	${OPENSSLSRC}/crypto/bn

CPPFLAGS+=	-I${OPENSSLSRC}
LDADD+=	${NETBSDSRCDIR}/lib/libcrypto/obj.i386/libcrypto.a

# do not install regression test programs
proginstall::
