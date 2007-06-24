#	$NetBSD: Makefile,v 1.95.2.1 2007/06/24 19:41:41 ghen Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc .WAIT libarch libbsdmalloc libbz2 libcompat libcrypt \
	libcurses libedit libevent libform libintl libkvm libl libm \
	libmenu libossaudio libpcap libpci libpmc libposix libpthread \
	libpthread_dbg libradius libresolv librmt librpcsvc librt libterm \
	libusbhid libutil libwrap liby libz

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

# XXX Crypto bits must be done before libtelnet.

.if (${MKCRYPTO} != "no")
# OpenSSL libraries.
SUBDIR+= libcrypto libdes

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
SUBDIR+= libroken libvers libcom_err libsl libss libasn1 libhdb \
	 libkadm5srv libkadm5clnt libgssapi libkafs

.if (${MKKERBEROS4} != "no")
# KTH Kerberos 4 libraries
SUBDIR+= libkdb libkadm libkstream
.endif	# MKKERBEROS4 != no
.endif	# MKKERBEROS != no

SUBDIR+=	libtelnet

# IPv6/IPsec
SUBDIR+=	libipsec

# I18N modules
SUBDIR+=	i18n_module


#
# Libraries that depend upon any listed previously
# (and those that depend upon these [and ...])
#
SUBDIR+=	.WAIT
SUBDIR+=	libcdk 

SUBDIR+=	libmagic	# depends on libz

.if (${MKCRYPTO} != "no")
SUBDIR+=	libssl
.endif
.if (${MKKERBEROS} != "no")
SUBDIR+=	libkrb5
.endif
.if (${MKKERBEROS4} != "no")
SUBDIR+=	libkrb
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+=	.WAIT libssh
.endif

.if (${MKPAM} != "no")
SUBDIR+=	.WAIT libpam
.endif

.include <bsd.subdir.mk>
