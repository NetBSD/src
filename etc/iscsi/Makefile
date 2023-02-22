#	$NetBSD: Makefile,v 1.3.80.1 2023/02/22 19:32:23 martin Exp $

.include <bsd.own.mk>

.if ${MKISCSI} != "no"

CONFIGFILES=    	auths targets
FILESDIR=       	/etc/iscsi
FILESMODE_auths=	0600
FILESMODE_targets=	0644

.endif

.include <bsd.files.mk>
.include <bsd.prog.mk>
