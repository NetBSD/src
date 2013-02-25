# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:30 tls Exp $

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
