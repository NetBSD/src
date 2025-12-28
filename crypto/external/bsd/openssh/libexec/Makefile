#	$NetBSD: Makefile,v 1.4 2025/12/28 15:34:59 thorpej Exp $

.include <bsd.own.mk>

SUBDIR=	sshd-auth sshd-session

.if (${MKFIDO2} != "no")
SUBDIR+= ssh-sk-helper
.endif

.include <bsd.subdir.mk>
