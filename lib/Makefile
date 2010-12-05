#	$NetBSD: Makefile,v 1.157 2010/12/05 20:11:21 pooka Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=		csu

.if (${MKGCC} != "no")
.if (${USE_COMPILERCRTSTUFF} == "yes")
SUBDIR+=	../gnu/lib/crtstuff4
.endif
SUBDIR+=	../gnu/lib/libgcc4
.endif

SUBDIR+=	libc
SUBDIR+=	.WAIT

#
# The SUBDIRs above are included here for completeness but should be built
# and installed prior to make(dependall) in this file, as libraries listed
# below will depend on versions from DESTDIR only.
#

SUBDIR+=libarch \
	libbsdmalloc libbluetooth libbz2 \
	libcompat libcrypt \
	libintl libkvm libm \
	libossaudio libpci libpmc libposix libprop libpthread \
	libpthread_dbg libpuffs libresolv librmt librpcsvc librt \
	librumpclient libterminfo libusbhid libutil libwrap liby libz

SUBDIR+=	../external/bsd/libpcap/lib

.if (${MKMDNS} != "no")
SUBDIR+=	../external/apache2/mDNSResponder/lib
.endif

SUBDIR+=	../external/bsd/am-utils/lib

.if (${MKATF} != "no")
SUBDIR+=	../external/bsd/atf/lib
.endif

SUBDIR+=	../external/bsd/flex/lib
SUBDIR+=	../external/bsd/libdwarf/lib
SUBDIR+=	../external/bsd/libelf/lib
SUBDIR+=	../external/bsd/libevent/lib

SUBDIR+=	../external/mit/lua/lib

SUBDIR+=	../external/public-domain/xz/lib

SUBDIR+=	../gnu/lib/libmalloc

.if (${MKGCC} != "no")
SUBDIR+=	../gnu/lib/libobjc4
.endif

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

SUBDIR+=	../external/bsd/bind/lib	# depends on libcrypto
SUBDIR+=	../external/bsd/fetch/lib	# depends on libssl
SUBDIR+=	../external/bsd/file/lib	# depends on libz

.if (${MKISCSI} != "no")
SUBDIR+=	../external/bsd/iscsi/lib	# depends on libpthread
.endif

SUBDIR+=	../external/bsd/libarchive/lib	# depends on libxz

.if (${MKLDAP} != "no")
SUBDIR+=	../external/bsd/openldap/lib
.endif

.if (${MKBINUTILS} != "no")
SUBDIR+=	../external/gpl3/binutils/lib	# libbfd depends on libz
.endif

.if (${MKGCC} != "no" && ${MKCXX} != "no")
SUBDIR+=	../gnu/lib/libstdc++-v3_4	# depends on libm
SUBDIR+=	../gnu/lib/libsupc++4
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
