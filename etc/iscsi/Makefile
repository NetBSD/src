#	$NetBSD: Makefile,v 1.7 2023/02/05 21:20:14 mlelstv Exp $

.include <bsd.own.mk>

# kernel iSCSI initiator
CONFIGFILES=    	volumes
FILESDIR=       	/etc/iscsi
FILESMODE_volumes=	0644

# userland iSCSI initiator and target
.if ${MKISCSI} != "no"

CONFIGFILES+=    	auths targets
FILESMODE_auths=	0600
FILESMODE_targets=	0644

.endif


.include <bsd.files.mk>
.include <bsd.prog.mk>
