# $NetBSD: Makefile,v 1.1.10.1 2014/08/10 07:07:22 tls Exp $

.include <bsd.init.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}

FILESDIR=	/usr/share/doc/reference/ref1/kyua/kyua-testers
FILESMODE=	444
FILES=		AUTHORS COPYING NEWS README
.endif

.include <bsd.files.mk>
