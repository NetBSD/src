#	$NetBSD: Makefile,v 1.1.10.1 2026/01/22 19:51:55 martin Exp $

PROG=	wg-keygen
MAN=	wg-keygen.8

PROGDPLIBS+=	ssh ${NETBSDSRCDIR}/crypto/external/bsd/openssh/lib

# libssh is private.
#
# XXX Should get this out of PROGDPLIBS.
PROGDPSUBDIRS+=	private

.include <bsd.prog.mk>
