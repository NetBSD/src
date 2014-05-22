# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:32 yamt Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/store

FILESDIR=	${KYUA_STOREDIR}
FILESMODE=	444
FILES=		migrate_v1_v2.sql \
		schema_v2.sql
.endif

.include <bsd.files.mk>
