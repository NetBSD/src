#	$NetBSD: Makefile,v 1.115 2007/05/28 12:06:17 tls Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc libssp .WAIT libarch libbsdmalloc libbluetooth libbz2 \
	libcompat libcrypt \
	libcurses libevent libform libintl libkvm libl libm \
	libmenu libossaudio libpcap libpci libpmc libposix libprop libpthread \
	libpthread_dbg libpuffs libresolv librmt librpcsvc librt libsdp \
	libterm libusbhid libutil libwrap liby libz

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
SUBDIR+= .WAIT libroken libvers libcom_err .WAIT libasn1

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
#==================== 1st library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libedit		# depends on libterm

SUBDIR+=	libmagic	# depends on libz

SUBDIR+=	libradius	# depends on libcrypto - only if ${MKCRYPTO}

.if (${MKISCSI} != "no")
SUBDIR+=	libiscsi	# depends on libpthread
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+=	libssl		# depends on libcrypto
SUBDIR+=	libssh		# depends on libcrypto
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+=	libkrb5		# depends on libcrypto and more
.endif

.if (${MKPUFFS} != "no")
SUBDIR+=	librefuse	# depends on libpuffs
.endif

#==================== 2nd library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
# Heimdal Kerberos 5 libraries depending on libkrb5 and more
SUBDIR+=	libgssapi libhdb .WAIT libkadm5srv libkadm5clnt libkafs libsl libss
.endif

.if (${MKPAM} != "no")
SUBDIR+=	.WAIT libpam		# depends on libkrb5, libkafs and more
.endif

.include <bsd.subdir.mk>
