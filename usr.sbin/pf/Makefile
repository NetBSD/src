#	$NetBSD: Makefile,v 1.5 2005/04/04 19:11:19 peter Exp $

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
