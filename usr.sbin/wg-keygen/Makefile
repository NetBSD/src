#	$NetBSD: Makefile,v 1.1.8.1 2026/11/04 23:29:03 perseant Exp $

PROG=	wg-keygen
MAN=	wg-keygen.8

PROGDPLIBS+=	ssh ${NETBSDSRCDIR}/crypto/external/bsd/openssh/lib

# libssh is private.
#
# XXX Should get this out of PROGDPLIBS.
PROGDPSUBDIRS+=	private

.include <bsd.prog.mk>
