#	$NetBSD: Makefile,v 1.59.2.2 2000/06/23 16:58:50 minoura Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

SUBDIR=	csu libarch libbz2 libc libcompat libcrypt libcurses libedit \
	libkvm libl libm libmenu libossaudio libpcap libposix \
	libresolv librmt librpcsvc libskey libterm libusb libutil libwrap \
	liby libz

# XXX Crypto bits must be done before libtelnet.

# OpenSSL libraries.  NOTE!  WE DO NOT TRAVERSE INTO libdes FOR A REASON!
SUBDIR+= libcrypto libssl

# Heimdal Kerberos 5 libraries
SUBDIR+= libroken libcom_err libsl libss libasn1 libkrb5 libhdb libkadm5 \
	 libkadm5srv libkadm5clnt libgssapi

# KTH Kerberos 4 libraries
SUBDIR+= libkrb libkdb libkadm libkafs
SUBDIR+= libkstream

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# rune encoding scheme modules
SUBDIR+=	localemod
# XXX: to test rune with old libc
# SUBDIR+=	libxpg4

.include <bsd.subdir.mk>
