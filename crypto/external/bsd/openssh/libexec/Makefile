#	$NetBSD: Makefile,v 1.3.2.4 2026/10/24 17:33:11 martin Exp $

.include <bsd.own.mk>

SUBDIR=	sshd-auth sshd-session

.if (${MKFIDO2} != "no")
SUBDIR+= ssh-sk-helper
.endif

.include <bsd.subdir.mk>
