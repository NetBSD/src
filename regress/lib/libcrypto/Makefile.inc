#	$NetBSD: Makefile.inc,v 1.5 2005/03/26 02:21:11 christos Exp $

.include <bsd.own.mk>

.include <bsd.own.mk>
CPPFLAGS+=-DOPENSSL_FIPS
CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

# do not install regression test programs
proginstall::
