#	$NetBSD: Makefile,v 1.3 2025/12/17 04:42:59 riastradh Exp $

PROG=	wg-keygen
MAN=	wg-keygen.8

PROGDPLIBS+=	ssh ${NETBSDSRCDIR}/crypto/external/bsd/openssh/lib

# libssh is private.
#
# XXX Should get this out of PROGDPLIBS.
PROGDPSUBDIRS+=	private

.include <bsd.prog.mk>
