#	$NetBSD: Makefile,v 1.1 2026/07/01 03:56:27 riastradh Exp $
#

.include <bsd.own.mk>

.if ${MKUNBOUND} != "no"

CONFIGFILES+=	unbound.conf
FILESDIR=	/etc/unbound
FILESMODE=	0644

.endif

.include <bsd.files.mk>
