#	$NetBSD: Makefile,v 1.6 2005/06/27 20:32:40 peter Exp $

.include <bsd.own.mk>

SUBDIR=		authpf
SUBDIR+=	ftp-proxy
SUBDIR+=	pfctl
SUBDIR+=	pflogd

SUBDIR+=	man

.if (${MKSHARE} != "no")
SUBDIR+=	examples
.endif

configinstall:
	${MAKEDIRTARGET} etc configinstall

.include <bsd.subdir.mk>
