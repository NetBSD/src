#	$NetBSD: Makefile.inc,v 1.3 2002/09/18 04:16:01 lukem Exp $

.include <bsd.own.mk>

CRYPTODIST=${NETBSDSRCDIR}/crypto/dist/openssl/crypto

# do not install regression test programs
proginstall::
