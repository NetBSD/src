# $NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:05:44 yamt Exp $

.include <bsd.own.mk>

SUBDIR= 	hello

.if ${MKSHARE} != "no"
FILES=		Makefile.inc.inst README
FILESDIR=	/usr/share/examples/asm
FILESNAME_Makefile.inc.inst=	Makefile.inc
.endif

.include <bsd.files.mk>
.include <bsd.subdir.mk>
