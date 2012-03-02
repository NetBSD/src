#	$NetBSD: Makefile,v 1.181.2.1 2012/03/02 18:27:53 riz Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91

.include <bsd.own.mk>

SUBDIR=		csu

.if (${MKGCC} != "no")
. if ${HAVE_GCC} == 4
.  if (${USE_COMPILERCRTSTUFF} == "yes")
SUBDIR+=	../gnu/lib/crtstuff4
.  endif
SUBDIR+=	../gnu/lib/libgcc4
. else
.  if (${USE_COMPILERCRTSTUFF} == "yes")
SUBDIR+=	../external/gpl3/gcc/lib/crtstuff
.  endif
SUBDIR+=	../external/gpl3/gcc/lib/libgcc
. endif
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
SUBDIR+=	../external/bsd/tre/lib
SUBDIR+=	../external/bsd/libdwarf/lib
SUBDIR+=	../external/bsd/libelf/lib
SUBDIR+=	../external/bsd/libevent/lib
SUBDIR+=	../external/bsd/liblzf/lib
SUBDIR+=	../external/bsd/libpcap/lib

SUBDIR+=	../external/mit/lua/lib
SUBDIR+=	../external/mit/expat/lib

SUBDIR+=	../external/public-domain/sqlite/lib
SUBDIR+=	../external/public-domain/xz/lib

SUBDIR+=	../gnu/lib/libmalloc

.if (${MKGCC} != "no")
. if ${HAVE_GCC} == 4
SUBDIR+=	../gnu/lib/libobjc4
. else
SUBDIR+=	../external/gpl3/gcc/lib/libobjc
SUBDIR+=	../external/gpl3/gcc/lib/libgomp
. endif
. if ${HAVE_GCC} >= 45 && !defined(MLIBDIR)
# Should probably move GMP, MPFR and MPC builds into the GCC >= 4.5
# specific build area, but we get better parallelism this way.
SUBDIR+=	../external/lgpl3/gmp/lib/libgmp
SUBDIR+=	../external/lgpl3/mpfr/lib/libmpfr
SUBDIR+=	../external/lgpl2/mpc/lib/libmpc
. endif
.endif

#
# Libraries that depend upon any listed previously
# (and those that depend upon these [and ...])
#
#==================== 1st library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libcurses	# depends on libterminfo
SUBDIR+=	libdm		# depends on libprop
SUBDIR+=	libedit		# depends on libterminfo
SUBDIR+=	libppath	# depends on libprop
SUBDIR+=	libperfuse	# depends on libpuffs
SUBDIR+=	libquota	# depends on libprop and librpcsvc
SUBDIR+=	librefuse	# depends on libpuffs
SUBDIR+=	librumpuser	# depends on libpthread
SUBDIR+=	librumphijack	# depends on librumpclient and libpthread

.if (${MKNPF} != "no")
SUBDIR+=	libnpf		# depends on libprop
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/openssl/lib # depends on libcrypt
.endif

SUBDIR+=	../external/bsd/file/lib	# depends on libz

.if (${MKISCSI} != "no")
SUBDIR+=	../external/bsd/iscsi/lib	# depends on libpthread
.endif

SUBDIR+=	../external/bsd/libarchive/lib	# depends on libxz

.if (${MKLVM} != "no")
SUBDIR+=	../external/gpl2/lvm2/lib	# depends on libprop
.endif

.if (${MKBINUTILS} != "no")
SUBDIR+=	../external/gpl3/binutils/lib	# libbfd depends on libz
.endif

.if (${MKGCC} != "no" && ${MKCXX} != "no")
. if ${HAVE_GCC} == 4
SUBDIR+=	../gnu/lib/libstdc++-v3_4	# depends on libm
SUBDIR+=	../gnu/lib/libsupc++4
. else
SUBDIR+=	../external/gpl3/gcc/lib/libstdc++-v3
SUBDIR+=	../external/gpl3/gcc/lib/libsupc++
. endif
.endif

#==================== 2nd library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libform		# depends on libcurses
SUBDIR+=	libmenu		# depends on libcurses
SUBDIR+=	libradius	# depends on libcrypto if (${MKCRYPTO} != "no")
SUBDIR+=	librump		# depends on librumpuser

.if (${MKKERBEROS} != "no")
SUBDIR+=	../crypto/external/bsd/heimdal/lib	# depends on libcrypto
							# libedit, libterminfo,
.endif

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

SUBDIR+=	librumpdev	# depends on librump
SUBDIR+=	librumpnet	# depends on librump
SUBDIR+=	librumpvfs	# depends on librump

.if (${MKPAM} != "no")
SUBDIR+=	libpam		# depends on heimdal
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+=	../crypto/external/bsd/libsaslc	# depends on heimdal, openssl
.endif

SUBDIR+=	../external/bsd/mdocml/lib

#==================== 4th library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libukfs		# depends on librumpvfs, librump

#==================== 5th library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	libp2k		# depends on libukfs, librumpvfs, libpuffs

# Lua bindings come last, they might depend on anything
SUBDIR+=	lua

.include <bsd.subdir.mk>
