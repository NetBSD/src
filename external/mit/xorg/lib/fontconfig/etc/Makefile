#	$NetBSD: Makefile,v 1.2.6.1 2009/09/17 04:24:30 snj Exp $

SUBDIR=	conf.d conf.avail

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.fontconfig}
CONFIGFILES= \
	fonts.conf \
	fonts.dtd

FILESDIR= \
	/etc/fonts

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
