# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:32 yamt Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/examples

FILESDIR=	${KYUA_EXAMPLESDIR}
FILESMODE=	444
FILES=		Kyuafile.top kyua.conf
.endif

.include <bsd.files.mk>
