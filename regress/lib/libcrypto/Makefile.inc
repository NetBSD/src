#	$NetBSD: Makefile.inc,v 1.4 2003/07/24 14:22:51 itojun Exp $

.include <bsd.own.mk>

.include <bsd.own.mk>
CRYPTODIST=${NETBSDSRCDIR}/crypto/dist
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"

# do not install regression test programs
proginstall::
