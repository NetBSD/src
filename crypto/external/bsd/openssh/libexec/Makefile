#	$NetBSD: Makefile,v 1.3.2.2 2026/05/07 17:49:29 martin Exp $

.include <bsd.own.mk>

SUBDIR=	sshd-auth sshd-session

.if (${MKFIDO2} != "no")
SUBDIR+= ssh-sk-helper
.endif

.include <bsd.subdir.mk>
