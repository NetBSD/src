#	$NetBSD: Makefile,v 1.1.12.2 2026/11/04 22:36:48 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	sshd-auth sshd-session

.if (${MKFIDO2} != "no")
SUBDIR+= ssh-sk-helper
.endif

.include <bsd.subdir.mk>
