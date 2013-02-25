# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:23 tls Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/examples

FILESDIR=	${KYUA_EXAMPLESDIR}
FILESMODE=	444
FILES=		Kyuafile.top kyua.conf
.endif

.include <bsd.files.mk>
