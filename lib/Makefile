#	$NetBSD: Makefile,v 1.72 2001/09/13 23:49:19 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libarch libbz2 libc libcdk libcompat libcrypt libcurses libedit \
	libform libintl libkvm libl libm libmenu libossaudio libpcap libpci \
	libposix libresolv librmt librpcsvc libskey libterm libusb libutil \
	libwrap liby libz

# XXX Crypto bits must be done before libtelnet.

.if (${MKCRYPTO} != "no")
# OpenSSL libraries.  NOTE!  WE DO NOT TRAVERSE INTO libdes FOR A REASON!
SUBDIR+= libcrypto libssl

# Heimdal Kerberos 5 libraries
SUBDIR+= libroken libvers libcom_err libsl libss libasn1 libkrb5 libhdb \
	 libkadm5srv libkadm5clnt libgssapi

# KTH Kerberos 4 libraries
SUBDIR+= libkrb libkdb libkadm libkafs
SUBDIR+= libkstream

.if (${MKCRYPTO_IDEA} != "no")
SUBDIR+= libcrypto_idea
.endif	# MKCRYPTO_IDEA != no

.if (${MKCRYPTO_RC5} != "no")
SUBDIR+= libcrypto_rc5
.endif	# MKCRYPTO_RC5 != no

.endif	# MKCRYPTO != no

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# multibyte locale
SUBDIR+=	runemodule

.include <bsd.subdir.mk>
