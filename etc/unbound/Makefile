#	$NetBSD: Makefile,v 1.1.2.2 2026/07/03 17:58:22 martin Exp $
#

.include <bsd.own.mk>

.if ${MKUNBOUND} != "no"

CONFIGFILES+=	unbound.conf
FILESDIR=	/etc/unbound
FILESMODE=	0644

.endif

.include <bsd.files.mk>
