#	$NetBSD: Makefile,v 1.4 2022/08/30 13:40:37 riastradh Exp $

.include <bsd.own.mk>

.if ${MKISCSI} != "no"

CONFIGFILES=    	auths targets
FILESDIR=       	/etc/iscsi
FILESMODE_auths=	0600
FILESMODE_targets=	0644

.endif

.include <bsd.files.mk>
.include <bsd.prog.mk>
