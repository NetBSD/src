#	$NetBSD: Makefile,v 1.4 2005/03/15 16:05:04 peter Exp $

.include <bsd.own.mk>

SUBDIR=		authpf
SUBDIR+=	ftp-proxy
SUBDIR+=	pfctl
# notyet SUBDIR+=	pflogd
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
