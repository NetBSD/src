#	$NetBSD: Makefile,v 1.59.2.1 2000/05/28 22:40:55 minoura Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SRCTOP=	..
.include <bsd.crypto.mk>

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit \
	libkvm libl libm libmenu libossaudio libpcap libposix \
	libresolv librmt librpcsvc libskey libterm libusb libutil libwrap \
	liby libz

# XXX Crypto bits must be dealt with before libtelnet.
.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/lib/Makefile.frag"
.endif

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# rune encoding scheme modules
SUBDIR+=	localemod
# XXX: to test rune with old libc
# SUBDIR+=	libxpg4

.include <bsd.subdir.mk>
