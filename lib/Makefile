#	$NetBSD: Makefile,v 1.54 1999/07/12 22:05:42 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SRCTOP=	..
.include <bsd.crypto.mk>

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit libkvm \
	libl libm libntp libossaudio libpcap libposix libresolv librmt \
	librpcsvc libskey libterm libusb libutil libwrap liby libz

# XXX Crypto bits must be dealt with before libtelnet.
.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/lib/Makefile.frag"
.endif

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

.include <bsd.subdir.mk>
