# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:39 yamt Exp $

.include <bsd.init.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}

FILESDIR=	/usr/share/doc/kyua-testers
FILESMODE=	444
FILES=		AUTHORS COPYING NEWS README
.endif

.include <bsd.files.mk>
