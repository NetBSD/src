#	$NetBSD: Makefile,v 1.10 2025/05/25 19:03:30 rillig Exp $

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
