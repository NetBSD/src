#	$NetBSD: Makefile.inc,v 1.3 2002/08/31 10:46:43 itojun Exp $

CRYPTODIST=${.CURDIR}/../../../../../crypto/dist/openssl/crypto
.PATH:	${CRYPTODIST}/bn

CPPFLAGS+=	-I${CRYPTODIST}/..

# do not install regression test programs
proginstall::
