#	$NetBSD: Makefile,v 1.76 2002/03/22 18:10:20 thorpej Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc .WAIT libarch libbz2 libcdk libcompat libcrypt libcurses libedit \
	libform libintl libkvm libl libm libmenu libossaudio libpcap libpci \
	libposix libresolv librmt librpcsvc libterm libusbhid libutil \
	libwrap liby libz

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

# XXX Crypto bits must be done before libtelnet.

.if (${MKCRYPTO} != "no")
# OpenSSL libraries.  NOTE!  WE DO NOT TRAVERSE INTO libdes FOR A REASON!
SUBDIR+= libcrypto libssl

.if (${MKCRYPTO_IDEA} != "no")
SUBDIR+= libcrypto_idea
.endif	# MKCRYPTO_IDEA != no

.if (${MKCRYPTO_RC5} != "no")
SUBDIR+= libcrypto_rc5
.endif	# MKCRYPTO_RC5 != no

.endif	# MKCRYPTO != no

.if (${MKKERBEROS} != "no")
# Heimdal Kerberos 5 libraries
SUBDIR+= libroken libvers libcom_err libsl libss libasn1 libkrb5 libhdb \
	 libkadm5srv libkadm5clnt libgssapi

# KTH Kerberos 4 libraries
SUBDIR+= libkrb libkdb libkadm libkafs
SUBDIR+= libkstream
.endif	# MKKERBEROS != no

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# I18N modules
SUBDIR+=	i18n_module

.include <bsd.subdir.mk>
