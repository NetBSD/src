#	$NetBSD: Makefile,v 1.151 2010/09/01 16:01:27 agc Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=	csu libc .WAIT libarch \
	libbsdmalloc libbluetooth libbz2 \
	libcompat libcrypt \
	libintl libkvm libm \
	libossaudio libpcap libpci libpmc libposix libprop libpthread \
	libpthread_dbg libpuffs libresolv librmt librpcsvc librt \
	libterminfo libusbhid libutil libwrap liby libz

SUBDIR+=../external/bsd/flex/lib

# libcurses needs libtermlib
SUBDIR+=	.WAIT libcurses

# libform and libmenu need libcurses
SUBDIR+=	.WAIT libform libmenu 

.if (${MKSKEY} != "no")
SUBDIR+= libskey
.endif

# XXX Crypto bits must be done before libtelnet.

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/openssl/lib
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


.if (${MKKERBEROS} != "no")
SUBDIR+=	libkrb5		# depends on libcrypto and more
.endif

SUBDIR+=	librefuse	# depends on libpuffs
SUBDIR+=	librumpuser	# depends on libpthread

.if (${MKCRYPTO} != "no")
# these depend on libcrypto*, libz and libbz2
SUBDIR+=	../crypto/external/bsd/openssh/lib
SUBDIR+=	../crypto/external/bsd/netpgp/libmj
SUBDIR+=	.WAIT
SUBDIR+=	../crypto/external/bsd/netpgp/lib
.endif

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

.if (${MKCRYPTO} != "no")
SUBDIR+=	librumpcrypto	# depends on librump
.endif
SUBDIR+=	librumpdev	# depends on librump
SUBDIR+=	librumpnet	# depends on librump
SUBDIR+=	librumpvfs	# depends on librump

#==================== 4th library dependency barrier ====================
SUBDIR+=	.WAIT
SUBDIR+=	libukfs		# depends on librump and librumpvfs

#==================== 5th library dependency barrier ====================
SUBDIR+=	.WAIT
SUBDIR+=	libp2k		# depends on libpuffs, librumpvfs and libukfs

.include <bsd.subdir.mk>
