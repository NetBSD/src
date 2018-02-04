#	$NetBSD: bsd.own.mk,v 1.1031 2018/02/04 03:19:53 christos Exp $

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"

#
# CPU model, derived from MACHINE_ARCH
#
MACHINE_CPU=	${MACHINE_ARCH:C/mipse[bl]/mips/:C/mips64e[bl]/mips/:C/sh3e[bl]/sh3/:S/coldfire/m68k/:S/m68000/m68k/:C/arm.*/arm/:C/earm.*/arm/:S/earm/arm/:S/powerpc64/powerpc/:S/aarch64eb/aarch64/:S/or1knd/or1k/:C/riscv../riscv/}

#
# Subdirectory used below ${RELEASEDIR} when building a release
#
RELEASEMACHINEDIR?=	${MACHINE}

#
# Subdirectory or path component used for the following paths:
#   distrib/${RELEASEMACHINE}
#   distrib/notes/${RELEASEMACHINE}
#   etc/etc.${RELEASEMACHINE}
# Used when building a release.
#
RELEASEMACHINE?=	${MACHINE}

#
# NEED_OWN_INSTALL_TARGET is set to "no" by pkgsrc/mk/bsd.pkg.mk to
# ensure that things defined by <bsd.own.mk> (default targets,
# INSTALL_FILE, etc.) are not conflicting with bsd.pkg.mk.
#
NEED_OWN_INSTALL_TARGET?=	yes

#
# This lists the platforms which do not have working in-tree toolchains.  For
# the in-tree gcc toolchain, this list is empty.
#
# If some future port is not supported by the in-tree toolchain, this should
# be set to "yes" for that port only.
#
# .if ${MACHINE} == "playstation2"
# TOOLCHAIN_MISSING?=	yes
# .endif

TOOLCHAIN_MISSING?=	no

.if ${MACHINE_CPU} == "aarch64" && !defined(EXTERNAL_TOOLCHAIN) && ${MKLLVM:Uyes} != "no"
MKLLVM?=	yes
HAVE_LLVM?=	yes
MKGCC?=		no
.endif

#
# GCC Using platforms.
#
.if ${MKGCC:Uyes} != "no"

#
# What GCC is used?
#
.if ${MACHINE_CPU} == "aarch64"
HAVE_GCC?=	0
.else
HAVE_GCC?=	5
.endif

#
# Platforms that can't run a modern GCC natively
.if ${MACHINE_ARCH} == "m68000"
MKGCCCMDS?=	no
.endif

#
# We import the old gcc as "gcc.old" when upgrading.  EXTERNAL_GCC_SUBDIR is
# set to the relevant subdirectory in src/external/gpl3 for his HAVE_GCC.
#
.if ${HAVE_GCC} == 6
EXTERNAL_GCC_SUBDIR?=	gcc
.elif ${HAVE_GCC} == 5
EXTERNAL_GCC_SUBDIR?=	gcc.old
.else
EXTERNAL_GCC_SUBDIR=?	/does/not/exist
.endif
.else
MKGCCCMDS?=	no
.endif

#
# What OpenSSL is used?
# 
.if ${MACHINE} == "amd64"
HAVE_OPENSSL?=  10
.else
HAVE_OPENSSL?=  10
.endif

.if ${HAVE_OPENSSL} == 11
EXTERNAL_OPENSSL_SUBDIR=openssl
.elif ${HAVE_OPENSSL} == 10
EXTERNAL_OPENSSL_SUBDIR=openssl.old
.else
EXTERNAL_OPENSSL_SUBDIR=/does/not/exist
.endif

.if !empty(MACHINE_ARCH:Mearm*)
_LIBC_COMPILER_RT.${MACHINE_ARCH}=	yes
.endif

_LIBC_COMPILER_RT.aarch64=	yes
_LIBC_COMPILER_RT.aarch64eb=	yes
_LIBC_COMPILER_RT.i386=		yes
_LIBC_COMPILER_RT.powerpc=	yes
_LIBC_COMPILER_RT.powerpc64=	yes
_LIBC_COMPILER_RT.sparc=	yes
_LIBC_COMPILER_RT.sparc64=	yes
_LIBC_COMPILER_RT.x86_64=	yes

.if ${HAVE_LLVM:Uno} == "yes" && ${_LIBC_COMPILER_RT.${MACHINE_ARCH}:Uno} == "yes"
HAVE_LIBGCC?=	no
.else
HAVE_LIBGCC?=	yes
.endif


# ia64 is not support
.if ${HAVE_LLVM:Uno} == "yes" || !empty(MACHINE_ARCH:Mearm*)
HAVE_LIBGCC_EH?=	no
.else
HAVE_LIBGCC_EH?=	yes
.endif

# Coverity does not like SSP
.if defined(COVERITY_TOP_CONFIG) || \
    ${MACHINE} == "alpha" || \
    ${MACHINE} == "hppa" || \
    ${MACHINE} == "ia64" || \
    ${MACHINE_CPU} == "mips"
HAVE_SSP?=	no
.else
HAVE_SSP?=	yes
.if !defined(NOFORT) && ${USE_FORT:Uno} != "no"
USE_SSP?=	yes
.endif
.endif

#
# What GDB is used?
#
HAVE_GDB?=	801

.if ${HAVE_GDB} == 801
EXTERNAL_GDB_SUBDIR=		gdb
.elif ${HAVE_GDB} == 712
EXTERNAL_GDB_SUBDIR=		gdb.old
.else
EXTERNAL_GDB_SUBDIR=		/does/not/exist
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=	227

.if ${HAVE_BINUTILS} == 227
EXTERNAL_BINUTILS_SUBDIR=	binutils
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

.if empty(.MAKEFLAGS:tW:M*-V .OBJDIR*)
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
PRINTOBJDIR=	${MAKE} -r -V .OBJDIR -f /dev/null xxx
.else
PRINTOBJDIR=	${MAKE} -V .OBJDIR
.endif
.else
PRINTOBJDIR=	echo /error/bsd.own.mk/PRINTOBJDIR # avoid infinite recursion
.endif

#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd "${.CURDIR}"; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree.
# * Set defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
# * Define _NETBSD_VERSION_DEPENDS.  Targets that depend on the
#   NetBSD version, or on variables defined at build time, can
#   declare a dependency on ${_NETBSD_VERSION_DEPENDS}.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd "${_SRC_TOP_}" && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	_SRC_TOP_OBJ_
.endif

_NETBSD_VERSION_DEPENDS=	${_SRC_TOP_OBJ_}/params
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/sys/param.h
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/newvers.sh
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/osrelease.sh
${_SRC_TOP_OBJ_}/params: .NOTMAIN .OPTIONAL # created by top level "make build"

.endif	# _SRC_TOP_ != ""		# }


.if (${_SRC_TOP_} != "") && \
    (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))
USETOOLS?=	yes
.endif
USETOOLS?=	no


.if ${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "mips64" || \
    ${MACHINE_ARCH} == "sh3"
.BEGIN:
	@echo "Must set MACHINE_ARCH to one of ${MACHINE_ARCH}eb or ${MACHINE_ARCH}el"
	@false
.elif defined(REQUIRETOOLS) && \
      (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)) && \
      ${USETOOLS} == "no"
.BEGIN:
	@echo "USETOOLS=no, but this component requires a version-specific host toolchain"
	@false
.endif

#
# Host platform information; may be overridden
#
.include <bsd.host.mk>

.if ${USETOOLS} == "yes"						# {

#
# Provide a default for TOOLDIR.
#
.if !defined(TOOLDIR)
TOOLDIR:=	${_SRC_TOP_OBJ_}/tooldir.${HOST_OSTYPE}
.MAKEOVERRIDES+= TOOLDIR
.endif

#
# This is the prefix used for the NetBSD-sourced tools.
#
_TOOL_PREFIX?=	nb

#
# If an external toolchain base is specified, use it.
#
.if defined(EXTERNAL_TOOLCHAIN)						# {
AR=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-as
LD=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

TOOL_CC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-g77
TOOL_OBJC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc

TOOL_CC.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
.else									# } {
# Define default locations for common tools.
.if ${USETOOLS_BINUTILS:Uyes} == "yes"					#  {
AR=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
LD=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-g77
TOOL_OBJC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
.endif									#  }

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang

# PCC supports C and Fortran
TOOL_CC.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcc
TOOL_CPP.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcpp
TOOL_CXX.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-p++

#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=

# Don't append another copy of sysroot (coming from COMPATCPPFLAGS etc.
# because it confuses Coverity. Still we need to cov-configure specially
# for each specific sysroot argument.
.if !defined(HOSTPROG) && !defined(HOSTLIB)
.  if ${DESTDIR} != ""
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=${DESTDIR}
.	endif
LDFLAGS+=	--sysroot=${DESTDIR}
.  else
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=/
.	endif
LDFLAGS+=	--sysroot=/
.  endif
.endif
.endif	# EXTERNAL_TOOLCHAIN						# }

DBSYM=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
ELF2AOUT=	${TOOLDIR}/bin/${_TOOL_PREFIX}m68k-elf2aout
ELF2ECOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
INSTALL=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install
LEX=		${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=		CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=		NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=		CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
MKDEPCXX=	CC=${CXX:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
PAXCTL=		${TOOLDIR}/bin/${_TOOL_PREFIX}paxctl
TSORT=		${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=		${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AMIGAAOUT2BB=	${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-aout2bb
TOOL_AMIGAELF2BB=	${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-elf2bb
TOOL_AMIGATXLT=		${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-txlt
TOOL_ASN1_COMPILE=	${TOOLDIR}/bin/${_TOOL_PREFIX}asn1_compile
TOOL_AWK=		${TOOLDIR}/bin/${_TOOL_PREFIX}awk
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=		${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_CLANG_TBLGEN=		${TOOLDIR}/bin/${_TOOL_PREFIX}clang-tblgen
TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_CTFCONVERT=	${TOOLDIR}/bin/${_TOOL_PREFIX}ctfconvert
TOOL_CTFMERGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctfmerge
TOOL_CVSLATEST=		${TOOLDIR}/bin/${_TOOL_PREFIX}cvslatest
TOOL_DB=		${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_DISKLABEL=		${TOOLDIR}/bin/nbdisklabel
TOOL_DTC=		${TOOLDIR}/bin/${_TOOL_PREFIX}dtc
TOOL_EQN=		${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FDISK=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-fdisk
TOOL_FGEN=		${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENASSYM=		${TOOLDIR}/bin/${_TOOL_PREFIX}genassym
TOOL_GENCAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GMAKE=		${TOOLDIR}/bin/${_TOOL_PREFIX}gmake
TOOL_GPT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gpt
# grep exists in src/tools, but is not hooked up into the build.
#TOOL_GREP=		${TOOLDIR}/bin/${_TOOL_PREFIX}grep
TOOL_GREP=		grep
TOOL_GROFF=		PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff
TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_HP300MKBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}hp300-mkboot
TOOL_HPPAMKBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}hppa-mkboot
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_JOIN=		${TOOLDIR}/bin/${_TOOL_PREFIX}join
TOOL_LLVM_TBLGEN=		${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen
TOOL_M4=		${TOOLDIR}/bin/${_TOOL_PREFIX}m4
TOOL_MACPPCFIXCOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}macppc-fixcoff
TOOL_MAKEFS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makefs
TOOL_MAKEINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}makeinfo
TOOL_MAKEKEYS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makekeys
TOOL_MAKESTRS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makestrs
TOOL_MAKEWHATIS=	${TOOLDIR}/bin/${_TOOL_PREFIX}makewhatis
TOOL_MANDOC_ASCII=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tascii
TOOL_MANDOC_HTML=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Thtml
TOOL_MANDOC_LINT=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tlint
TOOL_MDSETIMAGE=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-mdsetimage
TOOL_MENUC=		MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_M68KELF2AOUT=	${TOOLDIR}/bin/${_TOOL_PREFIX}m68k-elf2aout
TOOL_MIPSELF2ECOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
TOOL_MKCSMAPPER=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKNOD=		${TOOLDIR}/bin/${_TOOL_PREFIX}mknod
TOOL_MKTEMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MKUBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
TOOL_ELFTOSB=		${TOOLDIR}/bin/${_TOOL_PREFIX}elftosb
TOOL_MSGC=		MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_NBPERF=		${TOOLDIR}/bin/${_TOOL_PREFIX}perf
TOOL_NCDCS=		${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs
TOOL_PAX=		${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PIGZ=		${TOOLDIR}/bin/${_TOOL_PREFIX}pigz
TOOL_PKG_CREATE=	${TOOLDIR}/bin/${_TOOL_PREFIX}pkg_create
TOOL_POWERPCMKBOOTIMAGE=${TOOLDIR}/bin/${_TOOL_PREFIX}powerpc-mkbootimage
TOOL_PWD_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=		${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=	PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		RPCGEN_CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SED=		${TOOLDIR}/bin/${_TOOL_PREFIX}sed
TOOL_SLC=		${TOOLDIR}/bin/${_TOOL_PREFIX}slc
TOOL_SOELIM=		${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_SORTINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}sortinfo
TOOL_SPARKCRC=		${TOOLDIR}/bin/${_TOOL_PREFIX}sparkcrc
TOOL_STAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_STRFILE=		${TOOLDIR}/bin/${_TOOL_PREFIX}strfile
TOOL_SUNLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}sunlabel
TOOL_TBL=		${TOOLDIR}/bin/${_TOOL_PREFIX}tbl
TOOL_TIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}tic
TOOL_UUDECODE=		${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=		${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_VFONTEDPR=		${TOOLDIR}/libexec/${_TOOL_PREFIX}vfontedpr
TOOL_ZIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}zic

.else	# USETOOLS != yes						# } {

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		clang
TOOL_CPP.clang=		clang-cpp
TOOL_CXX.clang=		clang++
TOOL_OBJC.clang=	clang

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=	gcc
TOOL_CPP.gcc=	cpp
TOOL_CXX.gcc=	c++
TOOL_FC.gcc=	g77
TOOL_OBJC.gcc=	gcc

# PCC supports C and Fortran
TOOL_CC.pcc=		pcc
TOOL_CPP.pcc=		pcpp
TOOL_CXX.pcc=		p++

TOOL_AMIGAAOUT2BB=	amiga-aout2bb
TOOL_AMIGAELF2BB=	amiga-elf2bb
TOOL_AMIGATXLT=		amiga-txlt
TOOL_ASN1_COMPILE=	asn1_compile
TOOL_AWK=		awk
TOOL_CAP_MKDB=		cap_mkdb
TOOL_CAT=		cat
TOOL_CKSUM=		cksum
TOOL_CLANG_TBLGEN=	clang-tblgen
TOOL_COMPILE_ET=	compile_et
TOOL_CONFIG=		config
TOOL_CRUNCHGEN=		crunchgen
TOOL_CTAGS=		ctags
TOOL_CTFCONVERT=	ctfconvert
TOOL_CTFMERGE=		ctfmerge
TOOL_CVSLATEST=		cvslatest
TOOL_DB=		db
TOOL_DISKLABEL=		disklabel
TOOL_EQN=		eqn
TOOL_FDISK=		fdisk
TOOL_FGEN=		fgen
TOOL_GENASSYM=		genassym
TOOL_GENCAT=		gencat
TOOL_GMAKE=		gmake
TOOL_GPT=		gpt
TOOL_GREP=		grep
TOOL_GROFF=		groff
TOOL_HEXDUMP=		hexdump
TOOL_HP300MKBOOT=	hp300-mkboot
TOOL_HPPAMKBOOT=	hppa-mkboot
TOOL_INDXBIB=		indxbib
TOOL_INSTALLBOOT=	installboot
TOOL_INSTALL_INFO=	install-info
TOOL_JOIN=		join
TOOL_LLVM_TBLGEN=	llvm-tblgen
TOOL_M4=		m4
TOOL_MACPPCFIXCOFF=	macppc-fixcoff
TOOL_MAKEFS=		makefs
TOOL_MAKEINFO=		makeinfo
TOOL_MAKEKEYS=		makekeys
TOOL_MAKESTRS=		makestrs
TOOL_MAKEWHATIS=	/usr/libexec/makewhatis
TOOL_MANDOC_ASCII=	mandoc -Tascii
TOOL_MANDOC_HTML=	mandoc -Thtml
TOOL_MANDOC_LINT=	mandoc -Tlint
TOOL_MDSETIMAGE=	mdsetimage
TOOL_MENUC=		menuc
TOOL_M68KELF2AOUT=	m68k-elf2aout
TOOL_MIPSELF2ECOFF=	mips-elf2ecoff
TOOL_MKCSMAPPER=	mkcsmapper
TOOL_MKESDB=		mkesdb
TOOL_MKLOCALE=		mklocale
TOOL_MKMAGIC=		file
TOOL_MKNOD=		mknod
TOOL_MKTEMP=		mktemp
TOOL_MKUBOOTIMAGE=	mkubootimage
TOOL_ELFTOSB=		elftosb
TOOL_MSGC=		msgc
TOOL_MTREE=		mtree
TOOL_NBPERF=		nbperf
TOOL_NCDCS=		ncdcs
TOOL_PAX=		pax
TOOL_PIC=		pic
TOOL_PIGZ=		pigz
TOOL_PKG_CREATE=	pkg_create
TOOL_POWERPCMKBOOTIMAGE=	powerpc-mkbootimage
TOOL_PWD_MKDB=		pwd_mkdb
TOOL_REFER=		refer
TOOL_ROFF_ASCII=	nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		rpcgen
TOOL_SED=		sed
TOOL_SOELIM=		soelim
TOOL_SORTINFO=		sortinfo
TOOL_SPARKCRC=		sparkcrc
TOOL_STAT=		stat
TOOL_STRFILE=		strfile
TOOL_SUNLABEL=		sunlabel
TOOL_TBL=		tbl
TOOL_TIC=		tic
TOOL_UUDECODE=		uudecode
TOOL_VGRIND=		vgrind -f
TOOL_VFONTEDPR=		/usr/libexec/vfontedpr
TOOL_ZIC=		zic

.endif	# USETOOLS != yes						# }

# Standalone code should not be compiled with PIE or CTF
# Should create a better test
.if defined(BINDIR) && ${BINDIR} == "/usr/mdec"
NOPIE=			# defined
NOCTF=			# defined
.elif ${MACHINE} == "sun2"
NOPIE=			# we don't have PIC, so no PIE
.endif

# Fallback to ensure that all variables are defined to something
TOOL_CC.false=		false
TOOL_CPP.false=		false
TOOL_CXX.false=		false
TOOL_FC.false=		false
TOOL_OBJC.false=	false

AVAILABLE_COMPILER?=	${HAVE_PCC:Dpcc} ${HAVE_LLVM:Dclang} ${HAVE_GCC:Dgcc} ${EXTERNAL_TOOLCHAIN:Dgcc} false

.for _t in CC CPP CXX FC OBJC
ACTIVE_${_t}=	${AVAILABLE_COMPILER:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@:[1]}
SUPPORTED_${_t}=${AVAILABLE_COMPILER:Nfalse:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@}
.endfor
# make bugs prevent moving this into the .for loop
CC=		${TOOL_CC.${ACTIVE_CC}}
CPP=		${TOOL_CPP.${ACTIVE_CPP}}
CXX=		${TOOL_CXX.${ACTIVE_CXX}}
FC=		${TOOL_FC.${ACTIVE_FC}}
OBJC=		${TOOL_OBJC.${ACTIVE_OBJC}}

# For each ${MACHINE_CPU}, list the ports that use it.
MACHINES.aarch64=	evbarm64
MACHINES.alpha=		alpha
MACHINES.arm=		acorn32 cats epoc32 evbarm hpcarm \
			iyonix netwinder shark zaurus
MACHINES.coldfire=	evbcf
MACHINES.i386=		i386
MACHINES.ia64=		ia64
MACHINES.hppa=		hppa
MACHINES.m68000=	sun2
MACHINES.m68k=		amiga atari cesfic hp300 luna68k mac68k \
			news68k next68k sun3 x68k
MACHINES.mips=		arc cobalt algor cobalt emips evbmips ews4800mips \
			hpcmips mipsco newsmips pmax sbmips sgimips
MACHINES.or1k=		or1k
MACHINES.powerpc=	amigappc bebox evbppc ibmnws macppc mvmeppc \
			ofppc prep rs6000 sandpoint
MACHINES.riscv=		riscv
MACHINES.sh3=		dreamcast evbsh3 hpcsh landisk mmeye
MACHINES.sparc=		sparc sparc64
MACHINES.sparc64=	sparc64
MACHINES.vax=		vax
MACHINES.x86_64=	amd64

# for crunchide & ldd, define the OBJECT_FMTS used by a MACHINE_ARCH
#
OBJECT_FMTS=
.if	${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "ia64"
OBJECT_FMTS+=	elf32
.endif
.if	${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH:M*64*} != ""
OBJECT_FMTS+=	elf64
.endif

# OBJCOPY flags to create a.out binaries for old firmware
# shared among src/distrib and ${MACHINE}/conf/Makefile.${MACHINE}.inc
.if ${MACHINE_CPU} == "arm"
OBJCOPY_ELF2AOUT_FLAGS?=	\
	-O a.out-arm-netbsd	\
	-R .ident		\
	-R .ARM.attributes	\
	-R .ARM.exidx		\
	-R .ARM.extab		\
	-R .SUNW_ctf		\
	-R .arm.atpcs		\
	-R .comment		\
	-R .debug_abbrev	\
	-R .debug_aranges	\
	-R .debug_info		\
	-R .debug_line		\
	-R .debug_frame		\
	-R .debug_loc		\
	-R .debug_pubnames	\
	-R .debug_pubtypes	\
	-R .debug_ranges	\
	-R .debug_str		\
	-R .debug_macinfo	\
	-R .eh_frame		\
	-R .note.netbsd.ident
.endif

#
# Targets to check if DESTDIR or RELEASEDIR is provided
#
.if !target(check_DESTDIR)
check_DESTDIR: .PHONY .NOTMAIN
.if !defined(DESTDIR)
	@echo "setenv DESTDIR before doing that!"
	@false
.else
	@true
.endif
.endif

.if !target(check_RELEASEDIR)
check_RELEASEDIR: .PHONY .NOTMAIN
.if !defined(RELEASEDIR)
	@echo "setenv RELEASEDIR before doing that!"
	@false
.else
	@true
.endif
.endif

#
# Where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj (for example).
#
BSDSRCDIR?=	/usr/src
BSDOBJDIR?=	/usr/obj
NETBSDSRCDIR?=	${BSDSRCDIR}

BINGRP?=	wheel
BINOWN?=	root
BINMODE?=	555
NONBINMODE?=	444

# These are here mainly because we don't want suid root in case
# a Makefile defines BINMODE.
RUMPBINGRP?=	wheel
RUMPBINOWN?=	root
RUMPBINMODE?=	555
RUMPNONBINMODE?=444

MANDIR?=	/usr/share/man
MANGRP?=	wheel
MANOWN?=	root
MANMODE?=	${NONBINMODE}
MANINSTALL?=	${_MANINSTALL}

INFODIR?=	/usr/share/info
INFOGRP?=	wheel
INFOOWN?=	root
INFOMODE?=	${NONBINMODE}

LIBDIR?=	/usr/lib

LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=	${BINGRP}
LIBOWN?=	${BINOWN}
LIBMODE?=	${NONBINMODE}

DOCDIR?=	/usr/share/doc
DOCGRP?=	wheel
DOCOWN?=	root
DOCMODE?=	${NONBINMODE}

NLSDIR?=	/usr/share/nls
NLSGRP?=	wheel
NLSOWN?=	root
NLSMODE?=	${NONBINMODE}

KMODULEGRP?=	wheel
KMODULEOWN?=	root
KMODULEMODE?=	${NONBINMODE}

LOCALEDIR?=	/usr/share/locale
LOCALEGRP?=	wheel
LOCALEOWN?=	root
LOCALEMODE?=	${NONBINMODE}

FIRMWAREDIR?=	/libdata/firmware
FIRMWAREGRP?=	wheel
FIRMWAREOWN?=	root
FIRMWAREMODE?=	${NONBINMODE}

DEBUGDIR?=	/usr/libdata/debug
DEBUGGRP?=	wheel
DEBUGOWN?=	root
DEBUGMODE?=	${NONBINMODE}

MKDIRMODE?=	0755
MKDIRPERM?=	-m ${MKDIRMODE}

#
# Data-driven table using make variables to control how
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
#
# OBJECT_FMT:		currently either "ELF" or "a.out".
#
# All platforms are ELF.
#
OBJECT_FMT=	ELF

#
# If this platform's toolchain is missing, we obviously cannot build it.
#
.if ${TOOLCHAIN_MISSING} != "no"
MKBINUTILS:= no
MKGDB:= no
MKGCC:= no
.endif

#
# If we are using an external toolchain, we can still build the target's
# binutils, but we cannot build GCC's support libraries, since those are
# tightly-coupled to the version of GCC being used.
#
.if defined(EXTERNAL_TOOLCHAIN)
MKGCC:= no
.endif

# No GDB support for aarch64
MKGDB.aarch64=	no
MKGDB.aarch64eb=no
MKGDB.or1k=	no
MKGDB.riscv32=	no
MKGDB.riscv64=	no

# No kernel modules for or1k (yet)
MKKMOD.or1k=	no
MKKMOD.riscv32=	no
MKKMOD.riscv64=	no

# No profiling for or1k (yet)
MKPROFILE.or1k=	no
MKPROFILE.riscv32=no
MKPROFILE.riscv64=no

#
# The m68000 port is incomplete.
#
.if ${MACHINE_ARCH} == "m68000"
NOPIC=		# defined
MKISCSI=	no
# XXX GCC 4 outputs mcount() calling sequences that try to load values
# from over 64KB away and this fails to assemble.
.if defined(HAVE_GCC)
NOPROFILE=	# defined
.endif
.endif

#
# The ia64 port is incomplete.
#
MKLINT.ia64=	no
MKGDB.ia64=	no

#
# On VAX using ELF, all objects are PIC, not just shared libraries,
# so don't build the _pic version. VAX has no native TLS support either,
# so differences between TLS models are not relevant.
#
MKPICLIB.vax=	no

#
# Location of the file that contains the major and minor numbers of the
# version of a shared library.  If this file exists a shared library
# will be built by <bsd.lib.mk>.
#
SHLIB_VERSION_FILE?= ${.CURDIR}/shlib_version

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.aarch64eb=aarch64_be
GNU_ARCH.coldfire=m5407
GNU_ARCH.earm=arm
GNU_ARCH.earmhf=arm
GNU_ARCH.earmeb=armeb
GNU_ARCH.earmhfeb=armeb
GNU_ARCH.earmv4=armv4
GNU_ARCH.earmv4eb=armv4eb
GNU_ARCH.earmv5=arm
GNU_ARCH.earmv5eb=armeb
GNU_ARCH.earmv6=armv6
GNU_ARCH.earmv6hf=armv6
GNU_ARCH.earmv6eb=armv6eb
GNU_ARCH.earmv6hfeb=armv6eb
GNU_ARCH.earmv7=armv7
GNU_ARCH.earmv7hf=armv7
GNU_ARCH.earmv7eb=armv7eb
GNU_ARCH.earmv7hfeb=armv7eb
GNU_ARCH.i386=i486
GCC_CONFIG_ARCH.i386=i486
GCC_CONFIG_TUNE.i386=nocona
GCC_CONFIG_TUNE.x86_64=nocona
GNU_ARCH.m68000=m68010
GNU_ARCH.sh3eb=sh
GNU_ARCH.sh3el=shle
GNU_ARCH.mips64eb=mips64
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if (!empty(MACHINE_ARCH:Mearm*))
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf-${MACHINE_ARCH:C/eb//:C/v[4-7]//:S/earm/eabi/}
.elif (${MACHINE_GNU_ARCH} == "arm" || \
     ${MACHINE_GNU_ARCH} == "armeb" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_CPU} == "m68k" || \
     ${MACHINE_GNU_ARCH} == "sh" || \
     ${MACHINE_GNU_ARCH} == "shle" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "vax")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

.if !empty(MACHINE_ARCH:M*arm*)
# Flags to pass to CC for using the old APCS ABI on ARM for compat or stand.
ARM_APCS_FLAGS=	-mabi=apcs-gnu -mfloat-abi=soft
ARM_APCS_FLAGS+=${${ACTIVE_CC} == "gcc":? -marm :}
ARM_APCS_FLAGS+=${${ACTIVE_CC} == "clang":? -target ${MACHINE_GNU_ARCH}--netbsdelf -B ${TOOLDIR}/${MACHINE_GNU_PLATFORM}/bin :}
.endif

GENASSYM_CPPFLAGS+=	${${ACTIVE_CC} == "clang":? -no-integrated-as :}

TARGETS+=	all clean cleandir depend dependall includes \
		install lint obj regress tags html analyze describe \
		rumpdescribe
PHONY_NOTMAIN =	all clean cleandir depend dependall distclean includes \
		install lint obj regress beforedepend afterdepend \
		beforeinstall afterinstall realinstall realdepend realall \
		html subdir-all subdir-install subdir-depend analyze describe \
		rumpdescribe
.PHONY:		${PHONY_NOTMAIN}
.NOTMAIN:	${PHONY_NOTMAIN}

.if ${NEED_OWN_INSTALL_TARGET} != "no"
.if !target(install)
install:	beforeinstall .WAIT subdir-install realinstall .WAIT afterinstall
beforeinstall:
subdir-install:
realinstall:
afterinstall:
.endif
all:		realall subdir-all
subdir-all:
realall:
depend:		realdepend subdir-depend
subdir-depend:
realdepend:
distclean:	cleandir
cleandir:	clean

dependall:	.NOTMAIN realdepend .MAKE
	@cd "${.CURDIR}"; ${MAKE} realall
.endif

#
# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override in the make environment.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be set by Makefiles.
#
# Please keep etc/Makefile and share/man/man5/mk.conf.5 in sync
# with changes to the MK* variables here.
#

#
# Supported NO* options (if defined, MK* will be forced to "no",
# regardless of user's mk.conf setting).
#
# Source makefiles should set NO*, and not MK*, and must do so before
# including bsd.own.mk.
#
.for var in \
	NOCRYPTO NODOC NOHTML NOINFO NOLINKLIB NOLINT NOMAN NONLS NOOBJ NOPIC \
	NOPICINSTALL NOPROFILE NOSHARE NOSTATICLIB NODEBUGLIB
.if defined(${var})
MK${var:S/^NO//}:=	no
.endif
.endfor

#
# Older-style variables that enabled behaviour when set.
#
.for var in MANZ UNPRIVED UPDATE
.if defined(${var})
MK${var}:=	yes
.endif
.endfor

#
# MK* options which have variable defaults.
#
# aarch64eb is not yet supported.
#
.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "sparc64" \
    || ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el" \
    || ${MACHINE_ARCH} == "powerpc64" || ${MACHINE_ARCH} == "aarch64" \
    || ${MACHINE_ARCH} == "riscv64" || !empty(MACHINE_ARCH:Mearm*)
MKCOMPAT?=	yes
.else
# Don't let this build where it really isn't supported.
MKCOMPAT:=	no
.endif

.if ${MKCOMPAT} == "no"
MKCOMPATTESTS:=	no
MKCOMPATX11:=	no
.endif

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386" \
    || ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el" \
    || (${MACHINE} == "evbppc" && ${MACHINE_ARCH} == "powerpc")
MKCOMPATMODULES?=	yes
.else
MKCOMPATMODULES:=	no
.endif

#
# These platforms use softfloat by default.
#
.if ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
MKSOFTFLOAT?=	yes
.endif

#
# These platforms always use softfloat.
#
.if (${MACHINE_CPU} == "arm" && ${MACHINE_ARCH:M*hf*} == "") || \
    ${MACHINE_ARCH} == "coldfire" || ${MACHINE_CPU} == "or1k" || \
    ${MACHINE} == "emips" || ${MACHINE_CPU} == "sh3"
MKSOFTFLOAT=	yes
.endif

.if ${MACHINE} == "emips"
SOFTFLOAT_BITS=	32
.endif

#
# We want to build zfs only for amd64 by default for now.
#
.if ${MACHINE} == "amd64"
MKZFS?=		yes
.endif

#
# DTrace works on amd64, i386 and earm*
#
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    !empty(MACHINE_ARCH:Mearm*)
MKDTRACE?=	yes
MKCTF?=		yes
.endif

#
# PIE is enabled on many platforms by default.
#
# Coverity does not like PIE
.if !defined(COVERITY_TOP_CONFIG) && \
    (${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_CPU} == "arm" || \
    ${MACHINE_CPU} == "m68k" || \
    ${MACHINE_CPU} == "mips" || \
    ${MACHINE_CPU} == "sh3" || \
    ${MACHINE} == "sparc64")
MKPIE?=		yes
.else
MKPIE?=		no
.endif

#
# RELRO is enabled on i386 and amd64 by default
#
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
MKRELRO?=	partial
.else
MKRELRO?=	no
.endif

.if ${MACHINE_ARCH} == "x86_64"
MKSTATICPIE?=	no
.else
MKSTATICPIE?=	no
.endif

#
# MK* options which default to "yes".
#
_MKVARS.yes= \
	MKATF \
	MKBINUTILS \
	MKCOMPLEX MKCVS MKCXX \
	MKDOC MKDTC \
	MKDYNAMICROOT \
	MKGCC MKGDB MKGROFF \
	MKHESIOD MKHTML \
	MKIEEEFP MKINET6 MKINFO MKIPFILTER MKISCSI \
	MKKERBEROS \
	MKKMOD \
	MKLDAP MKLIBSTDCXX MKLINKLIB MKLVM \
	MKMAN MKMANDOC \
	MKMDNS \
	MKMAKEMANDB \
	MKNLS \
	MKNPF \
	MKOBJ \
	MKPAM MKPERFUSE \
	MKPF MKPIC MKPICINSTALL MKPICLIB MKPOSTFIX MKPROFILE \
	MKRUMP \
	MKSHARE MKSKEY MKSTATICLIB \
	MKUNBOUND \
	MKX11FONTS \
	MKYP
.for var in ${_MKVARS.yes}
${var}?=	${${var}.${MACHINE_ARCH}:Uyes}
.endfor

#
# MKGCCCMDS is only valid if we are building GCC so make it dependent on that.
#
_MKVARS.yes += MKGCCCMDS
MKGCCCMDS?=	${MKGCC}

#
# Exceptions to the above:
#

# Rump doesn't work yet on ia64
.if ${MACHINE} == "ia64"
MKRUMP=		no
.endif

# RUMP uses -nostdinc which coverity does not like
# It also does not use many new files, so disable it
.if defined(COVERITY_TOP_CONFIG)
MKRUMP=		no
.endif

#
# Build a dynamically linked /bin and /sbin, with the necessary shared
# libraries moved from /usr/lib to /lib and the shared linker moved
# from /usr/libexec to /lib
#
# Note that if the BINDIR is not /bin or /sbin, then we always use the
# non-DYNAMICROOT behavior (i.e. it is only enabled for programs in /bin
# and /sbin).  See <bsd.shlib.mk>.
#
# For ia64, ld.elf_so not yet implemented
.if ${MACHINE_ARCH} == "ia64"
MKDYNAMICROOT=	no
.endif

.if defined(MKREPRO)
MKARZERO ?= ${MKREPRO}
.endif

#
# MK* options which default to "no".  Note that MKZFS has a different
# default for some platforms, see above.  Please keep alphabetically
# sorted with at most one letter per line.
#
_MKVARS.no= \
	MKARZERO \
	MKBSDGREP MKBSDTAR \
	MKCATPAGES MKCOMPATTESTS MKCOMPATX11 MKCTF \
	MKDEBUG MKDEBUGLIB MKDTRACE \
	MKEXTSRC \
	MKGROFFHTMLDOC \
	MKKYUA \
	MKLIBCXX MKLLD MKLLDB MKLLVM MKLINT \
	MKMANZ MKMCLINKER \
	MKNSD \
	MKOBJDIRS \
	MKPCC MKPIGZGZIP \
	MKRADEONFIRMWARE MKREPRO \
	MKSLJIT MKSOFTFLOAT MKSTRIPIDENT \
	MKTEGRAFIRMWARE MKTPM \
	MKUNPRIVED MKUPDATE \
	MKX11 MKX11MOTIF MKXORG_SERVER \
	MKZFS
.for var in ${_MKVARS.no}
${var}?=	${${var}.${MACHINE_ARCH}:Uno}
.endfor

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "sparc" 
MKSLJIT=	yes
.endif

#
# Which platforms build the xorg-server drivers (as opposed
# to just Xnest and Xvfb.)
#
.if ${MACHINE} == "alpha"	|| \
    ${MACHINE} == "amd64"	|| \
    ${MACHINE} == "amiga"	|| \
    ${MACHINE} == "bebox"	|| \
    ${MACHINE} == "cats"	|| \
    ${MACHINE} == "dreamcast"	|| \
    ${MACHINE} == "ews4800mips"	|| \
    ${MACHINE} == "evbarm"	|| \
    ${MACHINE} == "evbmips"	|| \
    ${MACHINE} == "hp300"	|| \
    ${MACHINE} == "hpcarm"	|| \
    ${MACHINE} == "hpcmips"	|| \
    ${MACHINE} == "hpcsh"	|| \
    ${MACHINE} == "i386"	|| \
    ${MACHINE} == "ibmnws"	|| \
    ${MACHINE} == "luna68k"	|| \
    ${MACHINE} == "mac68k"	|| \
    ${MACHINE} == "macppc"	|| \
    ${MACHINE} == "netwinder"	|| \
    ${MACHINE} == "newsmips"	|| \
    ${MACHINE} == "prep"	|| \
    ${MACHINE} == "ofppc"	|| \
    ${MACHINE} == "sgimips"	|| \
    ${MACHINE} == "shark"	|| \
    ${MACHINE} == "sparc"	|| \
    ${MACHINE} == "sparc64"	|| \
    ${MACHINE} == "vax"		|| \
    ${MACHINE} == "zaurus"
MKXORG_SERVER=yes
.else
.endif

# Only install the radeon firmware on DRM-happy systems.
.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386"
MKRADEONFIRMWARE=		yes
.endif

# Only install the tegra firmware on evbarm and evbarm64.
.if ${MACHINE} == "evbarm" || ${MACHINE} == "evbarm64"
MKTEGRAFIRMWARE=		yes
.endif

#
# Force some options off if their dependencies are off.
#

.if ${MKCXX} == "no"
MKATF:=		no
MKGROFF:=	no
MKKYUA:=	no
.endif

.if ${MKMAN} == "no"
MKCATPAGES:=	no
MKHTML:=	no
.endif

_MANINSTALL=	maninstall
.if ${MKCATPAGES} != "no"
_MANINSTALL+=	catinstall
.endif
.if ${MKHTML} != "no"
_MANINSTALL+=	htmlinstall
.endif

.if ${MKLINKLIB} == "no"
MKLINT:=	no
MKPICINSTALL:=	no
MKPROFILE:=	no
.endif

.if ${MKPIC} == "no"
MKPICLIB:=	no
.endif

.if ${MKOBJ} == "no"
MKOBJDIRS:=	no
.endif

.if ${MKSHARE} == "no"
MKCATPAGES:=	no
MKDOC:=		no
MKINFO:=	no
MKHTML:=	no
MKMAN:=		no
MKNLS:=		no
.endif

.if !empty(MACHINE_ARCH:Mearm*)
_NEEDS_LIBCXX.${MACHINE_ARCH}=	yes
.endif
_NEEDS_LIBCXX.aarch64=		yes
_NEEDS_LIBCXX.aarch64eb=	yes
_NEEDS_LIBCXX.i386=		yes
_NEEDS_LIBCXX.powerpc=		yes
_NEEDS_LIBCXX.powerpc64=	yes
_NEEDS_LIBCXX.sparc=		yes
_NEEDS_LIBCXX.sparc64=		yes
_NEEDS_LIBCXX.x86_64=		yes

.if ${MKLLVM} == "yes" && ${_NEEDS_LIBCXX.${MACHINE_ARCH}:Uno} == "yes"
MKLIBCXX:=	yes
.endif

#
# install(1) parameters.
#
COPY?=		-c
.if ${MKUPDATE} == "no"
PRESERVE?=	
.else
PRESERVE?=	-p
.endif
RENAME?=	-r
HRDLINK?=	-l h
SYMLINK?=	-l s

METALOG?=	${DESTDIR}/METALOG
METALOG.add?=	${TOOL_CAT} -l >> ${METALOG}
.if (${_SRC_TOP_} != "")	# only set INSTPRIV if inside ${NETBSDSRCDIR}
.if ${MKUNPRIVED} != "no"
INSTPRIV.unpriv=-U -M ${METALOG} -D ${DESTDIR} -h sha256
.else
INSTPRIV.unpriv=
.endif
INSTPRIV?=	${INSTPRIV.unpriv} -N ${NETBSDSRCDIR}/etc
.endif
STRIPFLAG?=	

.if ${NEED_OWN_INSTALL_TARGET} != "no"
INSTALL_DIR?=		${INSTALL} ${INSTPRIV} -d
INSTALL_FILE?=		${INSTALL} ${INSTPRIV} ${COPY} ${PRESERVE} ${RENAME}
INSTALL_LINK?=		${INSTALL} ${INSTPRIV} ${HRDLINK} ${RENAME}
INSTALL_SYMLINK?=	${INSTALL} ${INSTPRIV} ${SYMLINK} ${RENAME}
.endif

#
# Set defaults for the USE_xxx variables.
#

#
# USE_* options which default to "no" and will be forced to "no" if their
# corresponding MK* variable is set to "no".
#
.for var in USE_SKEY
.if (${${var:S/USE_/MK/}} == "no")
${var}:= no
.else
${var}?= no
.endif
.endfor

#
# USE_* options which default to "yes" unless their corresponding MK*
# variable is set to "no".
#
.for var in USE_HESIOD USE_INET6 USE_KERBEROS USE_LDAP USE_PAM USE_YP
.if (${${var:S/USE_/MK/}} == "no")
${var}:= no
.else
${var}?= yes
.endif
.endfor

#
# USE_* options which default to "yes".
#
.for var in USE_JEMALLOC
${var}?= yes
.endfor

#
# USE_* options which default to "no".
#
# For now, disable pigz as compressor by default
.for var in USE_PIGZGZIP USE_LIBTRE
${var}?= no
.endfor

#
# TOOL_GZIP and friends.  These might refer to TOOL_PIGZ or to the host gzip.
#
.if ${USE_PIGZGZIP} != "no"
TOOL_GZIP=		${TOOL_PIGZ}
GZIP_N_FLAG?=		-nT
.else
TOOL_GZIP=		gzip
GZIP_N_FLAG?=		-n
.endif
TOOL_GZIP_N=		${TOOL_GZIP} ${GZIP_N_FLAG}

#
# Where X11 sources are and where it is installed to.
#
.if !defined(X11SRCDIR)
.if exists(${NETBSDSRCDIR}/../xsrc)
X11SRCDIR!=		cd "${NETBSDSRCDIR}/../xsrc" && pwd
.else
X11SRCDIR=		/usr/xsrc
.endif
.endif # !defined(X11SRCDIR)

X11SRCDIR.local?=	${X11SRCDIR}/local
X11ROOTDIR?=		/usr/X11R7
X11BINDIR?=		${X11ROOTDIR}/bin
X11ETCDIR?=		/etc/X11
X11FONTDIR?=		${X11ROOTDIR}/lib/X11/fonts
X11INCDIR?=		${X11ROOTDIR}/include
X11LIBDIR?=		${X11ROOTDIR}/lib/X11
X11MANDIR?=		${X11ROOTDIR}/man
X11SHAREDIR?=		${X11ROOTDIR}/share
X11USRLIBDIR?=		${X11ROOTDIR}/lib${MLIBDIR:D/${MLIBDIR}}

#
# New modular-xorg based builds
#
X11SRCDIRMIT?=		${X11SRCDIR}/external/mit
.for _lib in \
	FS ICE SM X11 XScrnSaver XTrap Xau Xcomposite Xcursor Xdamage \
	Xdmcp Xevie Xext Xfixes Xfont Xfont2 Xft Xi Xinerama Xmu Xpresent Xpm \
	Xrandr Xrender Xres Xt Xtst Xv XvMC Xxf86dga Xxf86misc Xxf86vm drm \
	epoxy fontenc xkbfile xkbui Xaw Xfontcache pciaccess xcb xshmfence \
	pthread-stubs
X11SRCDIR.${_lib}?=		${X11SRCDIRMIT}/lib${_lib}/dist
.endfor

.for _proto in \
	xcmisc xext xf86bigfont bigreqs input kb x fonts fixes scrnsaver \
	xinerama dri2 dri3 render resource record video xf86dga xf86misc \
	xf86vidmode composite damage trap gl randr fontcache xf86dri \
	present xcb-
X11SRCDIR.${_proto}proto?=		${X11SRCDIRMIT}/${_proto}proto/dist
.endfor

# During transition from xorg-server 1.10 to 1.18
.if \
    ${MACHINE} == "alpha"	|| \
    ${MACHINE} == "amiga"	|| \
    ${MACHINE} == "ews4800mips"	|| \
    ${MACHINE} == "hpcarm"	|| \
    ${MACHINE} == "hpcmips"	|| \
    ${MACHINE} == "hpcsh"	|| \
    ${MACHINE} == "mac68k"	|| \
    ${MACHINE} == "netwinder"	|| \
    ${MACHINE} == "newsmips"	|| \
    ${MACHINE} == "sgimips"	|| \
    ${MACHINE} == "vax"		|| \
    ${MACHINE} == "zaurus"
HAVE_XORG_SERVER_VER?=110
.else
HAVE_XORG_SERVER_VER?=118
.endif

.if ${HAVE_XORG_SERVER_VER} == "118"
XORG_SERVER_SUBDIR?=xorg-server
. if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
HAVE_XORG_GLAMOR?=	yes
. endif
.else
XORG_SERVER_SUBDIR?=xorg-server.old
.endif

X11SRCDIR.xorg-server?=		${X11SRCDIRMIT}/${XORG_SERVER_SUBDIR}/dist
HAVE_XORG_GLAMOR?=	no

# Build glamor extension?

.for _dir in \
	xtrans fontconfig freetype evieext mkfontscale bdftopcf \
	xorg-cf-files imake xbiff xkeyboard-config \
	xbitmaps appres xeyes xev xedit sessreg pixman \
	beforelight bitmap editres makedepend fonttosfnt fslsfonts fstobdf \
	glu glw mesa-demos MesaGLUT MesaLib MesaLib7 \
	ico iceauth listres lndir \
	luit xproxymanagementprotocol mkfontdir oclock proxymngr rgb \
	rstart setxkbmap showfont smproxy twm viewres \
	x11perf xauth xcalc xclipboard \
	xclock xcmsdb xconsole xditview xdpyinfo xdriinfo xdm \
	xfd xf86dga xfindproxy xfontsel xfwp xgamma xgc xhost xinit \
	xkill xload xlogo xlsatoms xlsclients xlsfonts xmag xmessage \
	xmh xmodmap xmore xman xprop xrandr xrdb xrefresh xset \
	xsetmode xsetpointer xsetroot xsm xstdcmap xvidtune xvinfo \
	xwininfo xwud xkbprint xkbevd \
	xterm xwd xfs xfsinfo xtrap xkbutils xkbcomp \
	xinput xcb-util xorg-docs \
	font-adobe-100dpi font-adobe-75dpi font-adobe-utopia-100dpi \
	font-adobe-utopia-75dpi font-adobe-utopia-type1 \
	font-alias \
	font-bh-100dpi font-bh-75dpi font-bh-lucidatypewriter-100dpi \
	font-bh-lucidatypewriter-75dpi font-bh-ttf font-bh-type1 \
	font-bitstream-100dpi font-bitstream-75dpi font-bitstream-type1 \
	font-cursor-misc font-daewoo-misc font-dec-misc font-ibm-type1 \
	font-isas-misc font-jis-misc font-misc-misc font-mutt-misc \
	font-sony-misc font-util ttf-bitstream-vera encodings
X11SRCDIR.${_dir}?=		${X11SRCDIRMIT}/${_dir}/dist
.endfor

.for _i in \
	elographics keyboard mouse synaptics vmmouse void ws
X11SRCDIR.xf86-input-${_i}?=	${X11SRCDIRMIT}/xf86-input-${_i}/dist
.endfor

# xf86-video-modesetting move into the server build.
EXTRA_DRIVERS=
.if ${HAVE_XORG_SERVER_VER} == "118"
X11SRCDIR.xf86-video-modesetting=${X11SRCDIR.xorg-server}/hw/xfree86/drivers/modesetting
.else
EXTRA_DRIVERS=	modesetting 
.endif

.for _v in \
	ag10e amdgpu apm ark ast ati ati-kms chips cirrus crime \
	geode glint i128 i740 igs imstt intel intel-old \
	${EXTRA_DRIVERS} mach64 mga \
	neomagic newport nouveau nsc nv openchrome pnozz \
	r128 rendition \
	s3 s3virge savage siliconmotion sis suncg14 \
	suncg6 sunffb sunleo suntcx \
	tdfx tga trident tseng vesa vga vmware wsfb xgi
X11SRCDIR.xf86-video-${_v}?=	${X11SRCDIRMIT}/xf86-video-${_v}/dist
.endfor


X11DRI?=			yes
X11LOADABLE?=			yes


#
# Where extsrc sources are and where it is installed to.
#
.if !defined(EXTSRCSRCDIR)
.if exists(${NETBSDSRCDIR}/../extsrc)
EXTSRCSRCDIR!=		cd "${NETBSDSRCDIR}/../extsrc" && pwd
.else
EXTSRCSRCDIR=		/usr/extsrc
.endif
.endif # !defined(EXTSRCSRCDIR)

EXTSRCROOTDIR?=		/usr/ext
EXTSRCBINDIR?=		${EXTSRCROOTDIR}/bin
EXTSRCETCDIR?=		/etc/ext
EXTSRCINCDIR?=		${EXTSRCROOTDIR}/include
EXTSRCLIBDIR?=		${EXTSRCROOTDIR}/lib/ext
EXTSRCMANDIR?=		${EXTSRCROOTDIR}/man
EXTSRCUSRLIBDIR?=	${EXTSRCROOTDIR}/lib

#
# MAKEDIRTARGET dir target [extra make(1) params]
#	run "cd $${dir} && ${MAKEDIRTARGETENV} ${MAKE} [params] $${target}", with a pretty message
#
MAKEDIRTARGETENV?=
MAKEDIRTARGET=\
	@_makedirtarget() { \
		dir="$$1"; shift; \
		target="$$1"; shift; \
		case "$${dir}" in \
		/*)	this="$${dir}/"; \
			real="$${dir}" ;; \
		.)	this="${_THISDIR_}"; \
			real="${.CURDIR}" ;; \
		*)	this="${_THISDIR_}$${dir}/"; \
			real="${.CURDIR}/$${dir}" ;; \
		esac; \
		show=$${this:-.}; \
		echo "$${target} ===> $${show%/}$${1:+	(with: $$@)}"; \
		cd "$${real}" \
		&& ${MAKEDIRTARGETENV} ${MAKE} _THISDIR_="$${this}" "$$@" $${target}; \
	}; \
	_makedirtarget

#
# MAKEVERBOSE support.  Levels are:
#	0	Minimal output ("quiet")
#	1	Describe what is occurring
#	2	Describe what is occurring and echo the actual command
#	3	Ignore the effect of the "@" prefix in make commands
#	4	Trace shell commands using the shell's -x flag
#		
MAKEVERBOSE?=		2

.if ${MAKEVERBOSE} == 0
_MKMSG?=	@\#
_MKSHMSG?=	: echo
_MKSHECHO?=	: echo
.SILENT:
.elif ${MAKEVERBOSE} == 1
_MKMSG?=	@echo '   '
_MKSHMSG?=	echo '   '
_MKSHECHO?=	: echo
.SILENT:
.else	# MAKEVERBOSE >= 2
_MKMSG?=	@echo '\#  '
_MKSHMSG?=	echo '\#  '
_MKSHECHO?=	echo
.SILENT: __makeverbose_dummy_target__
.endif	# MAKEVERBOSE >= 2
.if ${MAKEVERBOSE} >= 3
.MAKEFLAGS:	-dl
.endif	# ${MAKEVERBOSE} >= 3
.if ${MAKEVERBOSE} >= 4
.MAKEFLAGS:	-dx
.endif	# ${MAKEVERBOSE} >= 4

_MKMSG_BUILD?=		${_MKMSG} "  build "
_MKMSG_CREATE?=		${_MKMSG} " create "
_MKMSG_COMPILE?=	${_MKMSG} "compile "
_MKMSG_FORMAT?=		${_MKMSG} " format "
_MKMSG_INSTALL?=	${_MKMSG} "install "
_MKMSG_LINK?=		${_MKMSG} "   link "
_MKMSG_LEX?=		${_MKMSG} "    lex "
_MKMSG_REMOVE?=		${_MKMSG} " remove "
_MKMSG_YACC?=		${_MKMSG} "   yacc "

_MKSHMSG_CREATE?=	${_MKSHMSG} " create "
_MKSHMSG_INSTALL?=	${_MKSHMSG} "install "

_MKTARGET_BUILD?=	${_MKMSG_BUILD} ${.CURDIR:T}/${.TARGET}
_MKTARGET_CREATE?=	${_MKMSG_CREATE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_COMPILE?=	${_MKMSG_COMPILE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_FORMAT?=	${_MKMSG_FORMAT} ${.CURDIR:T}/${.TARGET}
_MKTARGET_INSTALL?=	${_MKMSG_INSTALL} ${.TARGET}
_MKTARGET_LINK?=	${_MKMSG_LINK} ${.CURDIR:T}/${.TARGET}
_MKTARGET_LEX?=		${_MKMSG_LEX} ${.CURDIR:T}/${.TARGET}
_MKTARGET_REMOVE?=	${_MKMSG_REMOVE} ${.TARGET}
_MKTARGET_YACC?=	${_MKMSG_YACC} ${.CURDIR:T}/${.TARGET}

.if ${MKMANDOC} == "yes"
TARGETS+=	lintmanpages
.endif

TESTSBASE=	/usr/tests${MLIBDIR:D/${MLIBDIR}}

# Override with tools versions if needed
.if ${MKCTF:Uno} != "no" && !defined(NOCTF)
CTFCONVERT=	${TOOL_CTFCONVERT}
CTFMERGE=	${TOOL_CTFMERGE}
.endif

.endif	# !defined(_BSD_OWN_MK_)
