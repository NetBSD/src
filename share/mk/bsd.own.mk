#	$NetBSD: bsd.own.mk,v 1.343 2003/07/22 11:48:59 mrg Exp $

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"

#
# CPU model, derived from MACHINE_ARCH
#
MACHINE_CPU=	${MACHINE_ARCH:C/mipse[bl]/mips/:C/sh3e[bl]/sh3/:C/sh5e[bl]/sh5/:S/m68000/m68k/:S/armeb/arm/}

#
# NEED_OWN_INSTALL_TARGET is set to "no" by pkgsrc/mk/bsd.pkg.mk to
# ensure that things defined by <bsd.own.mk> (default targets,
# INSTALL_FILE, etc.) are not conflicting with bsd.pkg.mk.
#
NEED_OWN_INSTALL_TARGET?=	yes

#
# This lists the platforms which do not have working in-tree toolchains.
#
.if ${MACHINE_CPU} == "hppa" || \
    ${MACHINE_CPU} == "ns32k" || \
    ${MACHINE_CPU} == "sh5" || \
    ${MACHINE_CPU} == "x86_64"
TOOLCHAIN_MISSING?=	yes
.else
TOOLCHAIN_MISSING=	no
.endif

#
# XXX TEMPORARY: If ns32k and not using an external toolchain, then we have
# to use -idirafter rather than -isystem, because the compiler is too old
# to use -isystem.
#
.if ${MACHINE_CPU} == "ns32k" && !defined(EXTERNAL_TOOLCHAIN)
CPPFLAG_ISYSTEM=	-idirafter
.else
CPPFLAG_ISYSTEM=	-isystem
.endif

.if empty(.MAKEFLAGS:M-V*)
PRINTOBJDIR=	${MAKE} -V .OBJDIR
.else
PRINTOBJDIR=	echo # prevent infinite recursion
.endif

#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd ${.CURDIR}; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree, so set
# defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd ${_SRC_TOP_} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	_SRC_TOP_OBJ_
.endif

.endif	# _SRC_TOP_ != ""		# }


.if (${_SRC_TOP_} != "") && \
    (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN))
USETOOLS?=	yes
.endif
USETOOLS?=	no


.if ${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "sh3" || \
    ${MACHINE_ARCH} == "sh5"
.BEGIN:
	@echo "Must set MACHINE_ARCH to one of ${MACHINE_ARCH}eb or ${MACHINE_ARCH}el"
	@false
.elif defined(REQUIRETOOLS) && \
      (${TOOLCHAIN_MISSING} != "yes" || defined(EXTERNAL_TOOLCHAIN)) && \
      ${USETOOLS} == "no"
.BEGIN:
	@echo "USETOOLS=no, but this component requires a version-specific host toolchain"
	@false
.endif

#
# Host platform information; may be overridden
#
.if !defined(HOST_OSTYPE)
_HOST_OSNAME!=	uname -s
_HOST_OSREL!=	uname -r
_HOST_ARCH!=	uname -p 2>/dev/null || uname -m
HOST_OSTYPE:=	${_HOST_OSNAME}-${_HOST_OSREL:C/\([^\)]*\)//}-${_HOST_ARCH}
.MAKEOVERRIDES+= HOST_OSTYPE
.endif
HOST_CYGWIN=	${HOST_OSTYPE:MCYGWIN*}

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
SIZE=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRIP=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

CC=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
CPP=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-cpp
CXX=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-c++
FC=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-g77
OBJC=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
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
SIZE=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRIP=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip
.endif									#  }

.if ${USETOOLS_GCC:Uyes} == "yes"					#  {
CC=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
CPP=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
CXX=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-c++
FC=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-g77
OBJC=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
.endif									#  }
.endif	# EXTERNAL_TOOLCHAIN						# }

HOST_MKDEP=	${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep

DBSYM=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
ELF2ECOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
INSTALL=	STRIP=${STRIP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}install
LEX=		${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=		CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=		NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=		CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
TSORT=		${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=		${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_ASN1_COMPILE=	${TOOLDIR}/bin/${_TOOL_PREFIX}asn1_compile
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=		${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_DB=		${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_EQN=		${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FGEN=		${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENCAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GROFF=		PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff
TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_M4=		${TOOLDIR}/bin/${_TOOL_PREFIX}m4
TOOL_MAKEFS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makefs
TOOL_MAKEINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}makeinfo
TOOL_MAKEWHATIS=	${TOOLDIR}/bin/${_TOOL_PREFIX}makewhatis
TOOL_MDSETIMAGE=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-mdsetimage
TOOL_MENUC=		MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_MKCSMAPPER=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKTEMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MSGC=		MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_PAX=		${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PREPMKBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}prep-mkbootimage
TOOL_PWD_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=		${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=	PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SOELIM=		${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_STAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_SUNLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}sunlabel
TOOL_TBL=		${TOOLDIR}/bin/${_TOOL_PREFIX}tbl
TOOL_UUDECODE=		${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=		${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_ZIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}zic

.endif	# USETOOLS == yes						# }

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


.if ${USETOOLS} == "yes"						# {
#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=
.endif									# }

#
# Build a dynamically linked /bin and /sbin, with the necessary shared
# libraries moved from /usr/lib to /lib and the shared linker moved
# from /usr/libexec to /lib
#
# Note that if the BINDIR is not /bin or /sbin, then we always use the
# non-DYNAMICROOT behavior (i.e. it is only enabled for programs in /bin
# and /sbin).  See <bsd.shlib.mk>.
#
MKDYNAMICROOT?=	yes

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

MANDIR?=	/usr/share/man
MANGRP?=	wheel
MANOWN?=	root
MANMODE?=	${NONBINMODE}
MANINSTALL?=	maninstall catinstall

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
HTMLDOCDIR?=	/usr/share/doc/html
DOCGRP?=	wheel
DOCOWN?=	root
DOCMODE?=	${NONBINMODE}

NLSDIR?=	/usr/share/nls
NLSGRP?=	wheel
NLSOWN?=	root
NLSMODE?=	${NONBINMODE}

KMODDIR?=	/usr/lkm
KMODGRP?=	wheel
KMODOWN?=	root
KMODMODE?=	${NONBINMODE}

LOCALEDIR?=	/usr/share/locale
LOCALEGRP?=	wheel
LOCALEOWN?=	root
LOCALEMODE?=	${NONBINMODE}

#
# Data-driven table using make variables to control how
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
#
# OBJECT_FMT:		currently either "ELF" or "a.out".
#
# All platforms are ELF, except for ns32k (which does not yet have
# an ELF BFD back-end).
#
.if ${MACHINE_ARCH} == "ns32k"
OBJECT_FMT?=	a.out		# allow overrides, to ease transition
.else
OBJECT_FMT=	ELF
.endif

#
# If this platform's toolchain is missing, we obviously cannot build it.
#
.if ${TOOLCHAIN_MISSING} == "yes"
MKBFD:= no
MKGDB:= no
MKGCC:= no
.endif

#
# If we are using an external toolchain, we can still build the target's
# BFD stuff, but we cannot build GCC's support libraries, since those are
# tightly-coupled to the version of GCC being used.
#
.if defined(EXTERNAL_TOOLCHAIN)
MKGCC:= no
.endif

#
# The sh3 port is incomplete.
#
.if ${MACHINE_CPU} == "sh3" && !defined(HAVE_GCC3)
NOPIC=		# defined
.endif

#
# The sh5 port is incomplete.
#
.if ${MACHINE_CPU} == "sh5"
NOPROFILE=	# defined
.endif

#
# The m68000 port is incomplete.
#
.if ${MACHINE_ARCH} == "m68000"
NOPIC=		# defined
.endif

#
# The hppa port is incomplete.
#
.if ${MACHINE_ARCH} == "hppa"
NOLINT=		# defined
NOPROFILE=	# defined
.endif

#
# On the MIPS, all libs are compiled with ABIcalls (and are thus PIC),
# not just shared libraries, so don't build the _pic version.
#
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
MKPICLIB:=	no
.endif

#
# If the ns32k port is using an external toolchain, shared libraries
# are not yet supported.
#
.if ${MACHINE_ARCH} == "ns32k" && defined(EXTERNAL_TOOLCHAIN)
NOPIC=		# defined
.endif

#
# On VAX using ELF, all objects are PIC, not just shared libraries,
# so don't build the _pic version.
#
.if ${MACHINE_ARCH} == "vax" && ${OBJECT_FMT} == "ELF"
MKPICLIB:=	no
.endif

#
# Location of the file that contains the major and minor numbers of the
# version of a shared library.  If this file exists a shared library
# will be built by <bsd.lib.mk>.
#
SHLIB_VERSION_FILE?= ${.CURDIR}/shlib_version

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.m68000=m68010
GNU_ARCH.sh3eb=sh
GNU_ARCH.sh3el=shle
GNU_ARCH.sh5eb=sh5
GNU_ARCH.sh5el=sh5le
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if ${OBJECT_FMT} == "ELF" && \
    (${MACHINE_GNU_ARCH} == "arm" || \
     ${MACHINE_GNU_ARCH} == "armeb" || \
     ${MACHINE_ARCH} == "ns32k" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_ARCH} == "m68k" || \
     ${MACHINE_ARCH} == "m68000" || \
     ${MACHINE_GNU_ARCH} == "sh" || \
     ${MACHINE_GNU_ARCH} == "shle" || \
     ${MACHINE_GNU_ARCH} == "sh5" || \
     ${MACHINE_GNU_ARCH} == "sh5le" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "vax")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

TARGETS+=	all clean cleandir depend dependall includes \
		install lint obj regress tags html installhtml cleanhtml
.PHONY:		all clean cleandir depend dependall distclean includes \
		install lint obj regress tags beforedepend afterdepend \
		beforeinstall afterinstall realinstall realdepend realall \
		html installhtml cleanhtml subdir-all subdir-install subdir-depend

.if ${NEED_OWN_INSTALL_TARGET} == "yes"
.if !target(install)
install:	.NOTMAIN beforeinstall subdir-install realinstall afterinstall
beforeinstall:	.NOTMAIN
subdir-install:	.NOTMAIN beforeinstall
realinstall:	.NOTMAIN beforeinstall
afterinstall:	.NOTMAIN subdir-install realinstall
.endif
all:		.NOTMAIN realall subdir-all
subdir-all:	.NOTMAIN
realall:	.NOTMAIN
depend:		.NOTMAIN realdepend subdir-depend
subdir-depend:	.NOTMAIN
realdepend:	.NOTMAIN
distclean:	.NOTMAIN cleandir
cleandir:	.NOTMAIN clean

dependall:	.NOTMAIN realdepend .MAKE
	@cd ${.CURDIR}; ${MAKE} realall
.endif

#
# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override on the make commandline.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be set by Makefiles.
#

#
# Supported NO* options (if defined, MK* will be forced to "no",
# regardless of user's mk.conf setting).
#
.for var in CRYPTO DOC HTML LINKLIB LINT MAN NLS OBJ PIC PICINSTALL PROFILE \
	SHARE
.if defined(NO${var})
MK${var}:=	no
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
# MK* options which default to "yes".
#
.for var in BFD CATPAGES CRYPTO DOC GCC GDB HESIOD HTML IEEEFP INFO KERBEROS \
	LINKLIB LINT MAN NLS OBJ PIC PICINSTALL PICLIB PROFILE SHARE SKEY YP
MK${var}?=	yes
.endfor

#
# MK* options which default to "no".
#
.for var in CRYPTO_IDEA CRYPTO_MDC2 CRYPTO_RC5 MANZ OBJDIRS SOFTFLOAT \
	UNPRIVED UPDATE
MK${var}?=	no
.endfor

#
# Force some options off if their dependencies are off.
#

.if ${MKCRYPTO} == "no"
MKKERBEROS:=	no
.endif

.if ${MKMAN} == "no"
MKCATPAGES:=	no
MKHTML:=	no
.endif

.if ${MKLINKLIB} == "no"
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
INSTPRIV.unpriv=-U -M ${METALOG} -D ${DESTDIR}
.else
INSTPRIV.unpriv=
.endif
INSTPRIV?=	${INSTPRIV.unpriv} -N ${NETBSDSRCDIR}/etc
.endif
SYSPKGTAG?=	${SYSPKG:D-T ${SYSPKG}_pkg}
SYSPKGDOCTAG?=	${SYSPKG:D-T ${SYSPKG}-doc_pkg}
STRIPFLAG?=	-s

.if ${NEED_OWN_INSTALL_TARGET} == "yes"
INSTALL_DIR?=		${INSTALL} ${INSTPRIV} -d
INSTALL_FILE?=		${INSTALL} ${INSTPRIV} ${COPY} ${PRESERVE} ${RENAME}
INSTALL_LINK?=		${INSTALL} ${INSTPRIV} ${HRDLINK} ${RENAME}
INSTALL_SYMLINK?=	${INSTALL} ${INSTPRIV} ${SYMLINK} ${RENAME}
HOST_INSTALL_FILE?=	${INSTALL} ${COPY} ${PRESERVE} ${RENAME}
.endif

#
# Set defaults for the USE_xxx variables.  They all default to "yes"
# unless the corresponding MKxxx variable is set to "no".
#
.for var in HESIOD KERBEROS SKEY YP
.if (${MK${var}} == "no")
USE_${var}:= no
.else
USE_${var}?= yes
.endif
.endfor

#
# Use XFree86 4.x as default version on i386, amd64, macppc and cats.
#
.if ${MACHINE_ARCH} == "i386" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "macppc" || ${MACHINE} == "cats" || \
    ${MACHINE} == "sparc"
USE_XF86_4?=	yes
.endif

.endif		# _BSD_OWN_MK_
