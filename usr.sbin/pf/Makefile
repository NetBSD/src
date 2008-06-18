#	$NetBSD: Makefile,v 1.8 2008/06/18 09:06:28 yamt Exp $

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
