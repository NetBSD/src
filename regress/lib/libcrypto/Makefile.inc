#	$NetBSD: Makefile.inc,v 1.2 2002/08/18 14:59:34 itojun Exp $

CRYPTODIST=${.CURDIR}/../../../../crypto/dist/openssl/crypto

# do not install regression test programs
proginstall::
