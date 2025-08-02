#	$NetBSD: Makefile,v 1.9.58.1 2025/08/02 05:58:54 perseant Exp $

.include <bsd.own.mk>

SUBDIR=		authpf
SUBDIR+=	ftp-proxy
SUBDIR+=	pfctl
SUBDIR+=	pflogd
SUBDIR+=	pfs
SUBDIR+=	tftp-proxy

SUBDIR+=	man

.if (${MKSHARE} != "no")
SUBDIR+=	examples
.endif

configinstall: .MAKE
	${MAKEDIRTARGET} etc configinstall
	${MAKEDIRTARGET} etc/defaults configinstall

.include <bsd.subdir.mk>
