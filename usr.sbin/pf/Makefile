#	$NetBSD: Makefile,v 1.3 2004/11/14 20:27:13 yamt Exp $

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

configinstall:
	${MAKEDIRTARGET} etc configinstall

.include <bsd.subdir.mk>
