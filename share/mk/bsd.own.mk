#	$NetBSD: bsd.own.mk,v 1.37 1997/05/07 17:06:26 mycroft Exp $

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
MANINSTALL?=	catinstall

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

# No lint, for now.
NOLINT=

# Profiling doesn't work on PowerPC yet.
.if (${MACHINE_ARCH} == "powerpc")
NOPROFILE=
.endif

TARGETS+=	all clean cleandir depend includes install lint obj tags
.PHONY:		all clean cleandir depend includes install lint obj tags \
		beforedepend afterdepend beforeinstall afterinstall

.if !target(install)
install: afterinstall
afterinstall: beforeinstall
.if !target(beforeinstall)
beforeinstall:
.endif
.if !target(afterinstall)
afterinstall:
.endif
.endif
