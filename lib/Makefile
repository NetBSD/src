#	$NetBSD: Makefile,v 1.60 2000/06/16 16:18:46 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SRCTOP=	..
.include <bsd.crypto.mk>

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit \
	libkvm libl libm libmenu libossaudio libpcap libposix \
	libresolv librmt librpcsvc libskey libterm libusb libutil libwrap \
	liby libz

SUBDIR+= libcrypto libssl

# XXX Crypto bits must be dealt with before libtelnet.
.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/lib/Makefile.frag"
.endif

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

.include <bsd.subdir.mk>
