#	$NetBSD: Makefile,v 1.87 2003/07/23 20:48:51 itojun Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc .WAIT libarch libbsdmalloc libbz2 libcdk libcompat libcrypt \
	libcurses libedit libevent libform libintl libkvm libl libm libmagic \
	libmenu libossaudio libpcap libpci libpmc libposix libpthread \
	libpthread_dbg libresolv librmt librpcsvc librt libterm libusbhid \
	libutil libwrap liby libz

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

# XXX Crypto bits must be done before libtelnet.

.if (${MKCRYPTO} != "no")
# OpenSSL libraries.
SUBDIR+= libcrypto libssl libdes

.if (${MKCRYPTO_IDEA} != "no")
SUBDIR+= libcrypto_idea
.endif	# MKCRYPTO_IDEA != no

.if (${MKCRYPTO_MDC2} != "no")
SUBDIR+= libcrypto_mdc2
.endif	# MKCRYPTO_MDC2 != no

.if (${MKCRYPTO_RC5} != "no")
SUBDIR+= libcrypto_rc5
.endif	# MKCRYPTO_RC5 != no

.endif	# MKCRYPTO != no

.if (${MKKERBEROS} != "no")
# Heimdal Kerberos 5 libraries
SUBDIR+= libroken libvers libcom_err libsl libss libasn1 libkrb5 libhdb \
	 libkadm5srv libkadm5clnt libgssapi libkafs
.endif	# MKKERBEROS != no

.if (${MKKERBEROS4} != "no")
# KTH Kerberos 4 libraries
SUBDIR+= libkrb libkdb libkadm
SUBDIR+= libkstream
.endif	# MKKERBEROS4 != no

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# I18N modules
SUBDIR+=	i18n_module

.include <bsd.subdir.mk>
