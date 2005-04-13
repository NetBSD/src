#	$NetBSD: Makefile,v 1.4.2.1 2005/04/13 15:49:52 tron Exp $

.include <bsd.own.mk>

SUBDIR=		authpf
SUBDIR+=	ftp-proxy
SUBDIR+=	pfctl
SUBDIR+=	pflogd
SUBDIR+=	spamd
SUBDIR+=	spamd-setup
SUBDIR+=	spamdb
SUBDIR+=	spamlogd

SUBDIR+=	man

.if (${MKSHARE} != "no")
SUBDIR+=	examples
.endif

configinstall:
	${MAKEDIRTARGET} etc configinstall

.include <bsd.subdir.mk>
