#	$NetBSD: Makefile,v 1.137 2009/05/12 18:07:54 plunky Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc .WAIT libarch \
	libbsdmalloc libbluetooth libbz2 \
	libcompat libcrypt \
	libcurses libevent libintl libkvm libl libm \
	libossaudio libpcap libpci libpmc libposix libprop libpthread \
	libpthread_dbg libpuffs libresolv librmt librpcsvc librt \
	libterm libusbhid libutil libwrap liby libz

# libform and libmenu need libcurses
SUBDIR+=	.WAIT libform libmenu 

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
SUBDIR+= .WAIT libhx509

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

SUBDIR+=	librefuse	# depends on libpuffs
SUBDIR+=	librumpuser	# depends on libpthread

#==================== 2nd library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
# Heimdal Kerberos 5 libraries depending on libkrb5 and more
SUBDIR+=	libheimntlm
SUBDIR+=	.WAIT libgssapi libhdb
SUBDIR+=	.WAIT libkadm5srv libkadm5clnt libkafs libsl libss
.endif

SUBDIR+=	librump		# depends on librumpuser

#==================== 3rd library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKPAM} != "no")
SUBDIR+=	libpam		# depends on libkrb5, libkafs and more
.endif

SUBDIR+=	librumpnet	# depends on librump
SUBDIR+=	librumpvfs	# depends on librump

#==================== 4th library dependency barrier ====================
SUBDIR+=	.WAIT
SUBDIR+=	libukfs		# depends on librump and librumpvfs

#==================== 5th library dependency barrier ====================
SUBDIR+=	.WAIT
SUBDIR+=	libp2k		# depends on libpuffs, librumpvfs and libukfs

.include <bsd.subdir.mk>
