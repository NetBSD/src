#	$NetBSD: bsd.own.mk,v 1.54.2.4 1998/11/07 01:12:25 cgd Exp $

.if defined(MAKECONF) && exists(${MAKECONF})
.include "${MAKECONF}"
.elif exists(/etc/mk.conf)
.include "/etc/mk.conf"
.endif

# Defining `SKEY' causes support for S/key authentication to be compiled in.
SKEY=		yes
# Defining `KERBEROS' causes support for Kerberos authentication to be
# compiled in.
#KERBEROS=	yes
# Defining 'KERBEROS5' causes support for Kerberos5 authentication to be
# compiled in.
#KERBEROS5=	yes

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=	/usr/src
BSDOBJDIR?=	/usr/obj

BINGRP?=	bin
BINOWN?=	bin
BINMODE?=	555
NONBINMODE?=	444

# Define MANZ to have the man pages compressed (gzip)
#MANZ=		1

MANDIR?=	/usr/share/man
MANGRP?=	bin
MANOWN?=	bin
MANMODE?=	${NONBINMODE}
MANINSTALL?=	maninstall catinstall

LIBDIR?=	/usr/lib
LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=	${BINGRP}
LIBOWN?=	${BINOWN}
LIBMODE?=	${NONBINMODE}

DOCDIR?=        /usr/share/doc
DOCGRP?=	bin
DOCOWN?=	bin
DOCMODE?=       ${NONBINMODE}

NLSDIR?=	/usr/share/nls
NLSGRP?=	bin
NLSOWN?=	bin
NLSMODE?=	${NONBINMODE}

KMODDIR?=	/usr/lkm
KMODGRP?=	bin
KMODOWN?=	bin
KMODMODE?=	${NONBINMODE}

COPY?=		-c
STRIPFLAG?=	-s

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks

# don't try to generate PIC versions of libraries on machines
# which don't support PIC.
.if  (${MACHINE_ARCH} == "vax") || \
    ((${MACHINE_ARCH} == "mips") && defined(STATIC_TOOLCHAIN)) || \
    ((${MACHINE_ARCH} == "alpha") && defined(ECOFF_TOOLCHAIN)) || \
    (${MACHINE_ARCH} == "powerpc")
NOPIC=
.endif

# Data-driven table using make variables to control how 
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out".
# SHLIB_TYPE:		"ELF" or "a.out" or "" to force static libraries.
#
.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "powerpc")
OBJECT_FMT?=ELF
.else
OBJECT_FMT?=a.out
.endif


# No lint, for now.
.if !defined(NONOLINT)
NOLINT=
.endif

# Profiling doesn't work on PowerPC yet.
.if (${MACHINE_ARCH} == "powerpc")
NOPROFILE=
.endif

# GNU sources and packages sometimes see architecture names differently.
# This table maps an architecture name to its GNU counterpart.
# Use as so:  ${GNU_ARCH.${TARGET_ARCH}} or ${MACHINE_GNU_ARCH}
GNU_ARCH.alpha=alpha
GNU_ARCH.arm32=arm
GNU_ARCH.i386=i386
GNU_ARCH.m68k=m68k
GNU_ARCH.mipseb=mipseb
GNU_ARCH.mipsel=mipsel
GNU_ARCH.ns32k=ns32k
GNU_ARCH.powerpc=powerpc
GNU_ARCH.sparc=sparc
GNU_ARCH.vax=vax
# XXX temporary compatibility
GNU_ARCH.mips=mipsel
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}}

TARGETS+=	all clean cleandir depend includes install lint obj regress \
		tags
.PHONY:		all clean cleandir depend includes install lint obj regress \
		tags beforedepend afterdepend beforeinstall afterinstall \
		realinstall

# set NEED_install_TARGET, if it's not already set, to yes
# this is used by bsd.port.mk to stop "install" being defined
NEED_OWN_INSTALL_TARGET?=	yes

.if (${NEED_OWN_INSTALL_TARGET} == "yes")
.if !target(install)
install:	.NOTMAIN beforeinstall subdir-install realinstall afterinstall
beforeinstall:	.NOTMAIN
subdir-install:	.NOTMAIN beforeinstall
realinstall:	.NOTMAIN beforeinstall
afterinstall:	.NOTMAIN subdir-install realinstall
.endif
.endif
