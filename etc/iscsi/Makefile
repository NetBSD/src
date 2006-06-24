#	$NetBSD: Makefile,v 1.3 2006/06/24 05:14:01 mrg Exp $

.include <bsd.own.mk>

.if ${MKISCSI} != "no"

CONFIGFILES=    	auths targets
FILESDIR=       	/etc/iscsi
FILESMODE_auths=	0600

.endif

.include <bsd.files.mk>
.include <bsd.prog.mk>
