#	$NetBSD: Makefile,v 1.7.20.1 2008/06/23 04:32:13 wrstuden Exp $

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
