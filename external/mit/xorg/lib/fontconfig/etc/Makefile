#	$NetBSD: Makefile,v 1.2.12.1 2010/04/21 05:24:35 matt Exp $

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
