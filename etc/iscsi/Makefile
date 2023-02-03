#	$NetBSD: Makefile,v 1.5 2023/02/03 15:59:04 mlelstv Exp $

.include <bsd.own.mk>

.if ${MKISCSI} != "no"

CONFIGFILES=    	auths targets volumes
FILESDIR=       	/etc/iscsi
FILESMODE_auths=	0600
FILESMODE_targets=	0644

.endif

.include <bsd.files.mk>
.include <bsd.prog.mk>
