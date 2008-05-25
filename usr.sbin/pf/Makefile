#	$NetBSD: Makefile,v 1.7.18.1 2008/05/25 18:13:48 peter Exp $

.include <bsd.own.mk>

SUBDIR=		authpf
SUBDIR+=	ftp-proxy
SUBDIR+=	pfctl
SUBDIR+=	pflogd
SUBDIR+=	tftp-proxy

SUBDIR+=	man

.if (${MKSHARE} != "no")
SUBDIR+=	examples
.endif

configinstall:
	${MAKEDIRTARGET} etc configinstall
	${MAKEDIRTARGET} etc/defaults configinstall

.include <bsd.subdir.mk>
