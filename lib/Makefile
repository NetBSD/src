#	$NetBSD: Makefile,v 1.159 2011/01/17 20:39:03 agc Exp $
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

SUBDIR+=	i18n_module

SUBDIR+=	libarch \
		libbluetooth libbsdmalloc libbz2 \
		libcompat libcrypt \
		libintl libipsec libisns libkvm libm \
		libossaudio libpci libpmc libposix libprop libpthread \
		libpthread_dbg libpuffs libresolv librmt librpcsvc librt \
		librumpclient libtelnet libterminfo \
		libusbhid libutil libwrap liby libz

.if (${MKKERBEROS} != "no")
SUBDIR+=	libcom_err
SUBDIR+=	libvers
.endif

.if (${MKSKEY} != "no")
SUBDIR+=	libskey
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/netpgp/libmj
.endif

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
SUBDIR+=	../external/bsd/libpcap/lib

SUBDIR+=	../external/mit/lua/lib

SUBDIR+=	../external/public-domain/xz/lib

SUBDIR+=	../gnu/lib/libmalloc

.if (${MKGCC} != "no")
SUBDIR+=	../gnu/lib/libobjc4
.endif

#
# Libraries that depend upon any listed previously
# (and those that depend upon these [and ...])
#
#==================== 1st library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
SUBDIR+=	libasn1		# depends on libcom_err
SUBDIR+=	libroken	# depends on libutil, libcrypt
.endif

SUBDIR+=	libcurses	# depends on libterminfo
SUBDIR+=	libedit		# depends on libterminfo
SUBDIR+=	librefuse	# depends on libpuffs
SUBDIR+=	librumpuser	# depends on libpthread

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/openssl/lib # depends on libcrypt
.endif

SUBDIR+=	../external/bsd/file/lib	# depends on libz

.if (${MKISCSI} != "no")
SUBDIR+=	../external/bsd/iscsi/lib	# depends on libpthread
.endif

SUBDIR+=	../external/bsd/libarchive/lib	# depends on libxz

.if (${MKBINUTILS} != "no")
SUBDIR+=	../external/gpl3/binutils/lib	# libbfd depends on libz
.endif

.if (${MKGCC} != "no" && ${MKCXX} != "no")
SUBDIR+=	../gnu/lib/libstdc++-v3_4	# depends on libm
SUBDIR+=	../gnu/lib/libsupc++4
.endif

#==================== 2nd library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libform		# depends on libcurses

.if (${MKKERBEROS} != "no")
SUBDIR+=	libhx509	# depends on libcrypto, libasn1, libroken
SUBDIR+=	libsl		# depends on libedit, libterminfo
SUBDIR+=	libss		# depends on libedit, libterminfo, ...
.endif

SUBDIR+=	libmenu		# depends on libcurses
SUBDIR+=	libradius	# depends on libcrypto if (${MKCRYPTO} != "no")
SUBDIR+=	librump		# depends on librumpuser

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/openssh/lib # depends on libcrypto, libz
SUBDIR+=	../crypto/external/bsd/netpgp/lib  # depends on libcrypto, ...
.endif

SUBDIR+=	../external/bsd/bind/lib	# depends on libcrypto
SUBDIR+=	../external/bsd/fetch/lib	# depends on libssl

.if (${MKLDAP} != "no")
SUBDIR+=	../external/bsd/openldap/lib	# depends on libcrypto, ...
.endif

#==================== 3rd library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
SUBDIR+=	libkrb5		# depends on libhx509, libcrypto, libasn1, ...
.endif

SUBDIR+=	librumpdev	# depends on librump
SUBDIR+=	librumpnet	# depends on librump
SUBDIR+=	librumpvfs	# depends on librump

#==================== 4th library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
SUBDIR+=	libhdb		# depends on libkrb5, libasn1, libroken
SUBDIR+=	libheimntlm	# depends on libkrb5, libasn1, libroken
SUBDIR+=	libkafs		# depends on libkrb5, libroken
.endif

SUBDIR+=	libukfs		# depends on librumpvfs, librump

#==================== 5th library dependency barrier ====================
SUBDIR+=	.WAIT

.if (${MKKERBEROS} != "no")
SUBDIR+=	libgssapi	# depends on libheimntlm
SUBDIR+=	libkadm5clnt	# depends on libhdb, libkrb5, libroken
SUBDIR+=	libkadm5srv	# depends on libhdb, libkrb5, libroken
.endif

SUBDIR+=	libp2k		# depends on libukfs, librumpvfs, libpuffs

.if (${MKPAM} != "no")
SUBDIR+=	libpam		# depends on libkafs, libkrb5, ...
.endif

.include <bsd.subdir.mk>
