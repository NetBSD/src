#	$NetBSD: bsd.own.mk,v 1.100 1998/11/21 22:14:47 mycroft Exp $

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

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

BINGRP?=	wheel
BINOWN?=	root
BINMODE?=	555
NONBINMODE?=	444

# Define MANZ to have the man pages compressed (gzip)
#MANZ=		1

MANDIR?=	/usr/share/man
MANGRP?=	wheel
MANOWN?=	root
MANMODE?=	${NONBINMODE}
MANINSTALL?=	maninstall catinstall

LIBDIR?=	/usr/lib
LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=	${BINGRP}
LIBOWN?=	${BINOWN}
LIBMODE?=	${NONBINMODE}

DOCDIR?=        /usr/share/doc
DOCGRP?=	wheel
DOCOWN?=	root
DOCMODE?=       ${NONBINMODE}

NLSDIR?=	/usr/share/nls
NLSGRP?=	wheel
NLSOWN?=	root
NLSMODE?=	${NONBINMODE}

KMODDIR?=	/usr/lkm
KMODGRP?=	wheel
KMODOWN?=	root
KMODMODE?=	${NONBINMODE}

COPY?=		-c
.if defined(UPDATE)
PRESERVE?=	-p
.else
PRESERVE?=
.endif
STRIPFLAG?=	-s

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks

# XXX The next two are temporary until the transition to UVM is complete.

# Systems on which UVM is the standard VM system.
.if	(${MACHINE} == "alpha") || \
	(${MACHINE} == "amiga") || \
	(${MACHINE} == "arm32") || \
	(${MACHINE} == "atari") || \
	(${MACHINE} == "bebox") || \
	(${MACHINE} == "hp300") || \
	(${MACHINE} == "i386") || \
	(${MACHINE} == "mac68k") || \
	(${MACHINE} == "macppc") || \
	(${MACHINE} == "mvme68k") || \
	(${MACHINE} == "newsmips") || \
	(${MACHINE} == "next68k") || \
	(${MACHINE} == "ofppc") || \
	(${MACHINE} == "pc532") || \
	(${MACHINE} == "pmax") || \
	(${MACHINE} == "sparc") || \
	(${MACHINE} == "sparc64") || \
	(${MACHINE} == "vax")
UVM?=		yes
.endif

# Systems that use UVM's new pmap interface.
.if	(${MACHINE} == "alpha") || \
	(${MACHINE} == "i386") || \
	(${MACHINE} == "pc532") || \
	(${MACHINE} == "vax")
PMAP_NEW?=	yes
.endif

# don't try to generate PIC versions of libraries on machines
# which don't support PIC.
.if ((${MACHINE_ARCH} == "mips") && defined(STATIC_TOOLCHAIN)) || \
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
    (${MACHINE_ARCH} == "powerpc") || \
    (${MACHINE_ARCH} == "sparc64")
OBJECT_FMT?=ELF
.else
OBJECT_FMT?=a.out
.endif

# Profiling doesn't work on PowerPC yet.
.if (${MACHINE_ARCH} == "powerpc")
NOPROFILE=
.endif

# Some platforms are already transitioned to egcs.
.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "arm32") || \
    (${MACHINE_ARCH} == "i386") || \
    (${MACHINE_ARCH} == "ns32k") || \
    (${MACHINE_ARCH} == "mips") || \
    (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "vax")
USE_EGCS=1
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
GNU_ARCH.sparc64=sparc64
GNU_ARCH.vax=vax
# XXX temporary compatibility
GNU_ARCH.mips=mipsel
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}}

TARGETS+=	all clean cleandir depend distclean includes install lint obj \
		regress tags
.PHONY:		all clean cleandir depend distclean includes install lint obj \
		regress tags beforedepend afterdepend beforeinstall \
		afterinstall realinstall

# set NEED_OWN_INSTALL_TARGET, if it's not already set, to yes
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

.endif		# _BSD_OWN_MK_
