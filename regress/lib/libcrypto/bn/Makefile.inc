#	$NetBSD: Makefile.inc,v 1.4 2002/09/18 04:16:01 lukem Exp $

.include <bsd.own.mk>

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist/openssl/crypto
.PATH:	${CRYPTODIST}/bn

CPPFLAGS+=	-I${CRYPTODIST}/..

# do not install regression test programs
proginstall::
