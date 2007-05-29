#	$NetBSD: bsd.sys.mk,v 1.147 2007/05/29 21:24:57 tls Exp $
#
# Build definitions used for NetBSD source tree builds.

.if !defined(_BSD_SYS_MK_)
_BSD_SYS_MK_=1

.if defined(WARNS)
.if ${WARNS} > 0
CFLAGS+=	-Wall -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
#CFLAGS+=	-Wmissing-declarations -Wredundant-decls -Wnested-externs
# Add -Wno-sign-compare.  -Wsign-compare is included in -Wall as of GCC 3.3,
# but our sources aren't up for it yet. Also, add -Wno-traditional because
# gcc includes #elif in the warnings, which is 'this code will not compile
# in a traditional environment' warning, as opposed to 'this code behaves
# differently in traditional and ansi environments' which is the warning
# we wanted, and now we don't get anymore.
CFLAGS+=	-Wno-sign-compare -Wno-traditional
.endif
.if ${WARNS} > 1
CFLAGS+=	-Wreturn-type -Wswitch -Wshadow
.endif
.if ${WARNS} > 2
CFLAGS+=	-Wcast-qual -Wwrite-strings
CFLAGS+=	-Wextra -Wno-unused-parameter
CXXFLAGS+=	-Wabi
CXXFLAGS+=	-Wold-style-cast
CXXFLAGS+=	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wreorder \
		-Wno-deprecated -Wno-non-template-friend \
		-Woverloaded-virtual -Wno-pmf-conversions -Wsign-promo -Wsynth
.endif
.if ${WARNS} > 3 && ${HAVE_GCC} >= 3
CFLAGS+=	-std=gnu99
.endif
.endif

CPPFLAGS+=	${AUDIT:D-D__AUDIT__}
CFLAGS+=	${CWARNFLAGS} ${NOGCCERROR:D:U-Werror}
LINTFLAGS+=	${DESTDIR:D-d ${DESTDIR}/usr/include}

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "hppa") && \
	(${MACHINE_ARCH} != "mipsel") && (${MACHINE_ARCH} != "mipsel")

.if defined(USE_FORT) && (${USE_FORT} != "no")
USE_SSP=	yes
.if !defined(KERNSRCDIR) && !defined(KERN) # not for kernels nor kern modules
.if defined(LIB)
.if (${LIB} != "ssp") && (${LIB} != "c")
COPTS+=		-D_FORTIFY_SOURCE=2 -I ${DESTDIR}/usr/include/ssp
LIBDPLIBS+=	ssp ${NETBSDSRCDIR}/lib/libssp
.endif
.else
COPTS+=		-D_FORTIFY_SOURCE=2 -I ${DESTDIR}/usr/include/ssp
LDADD+=		-lssp
.endif
.endif
.endif

.if defined(USE_SSP) && (${USE_SSP} != "no") && (${BINDIR:Ux} != "/usr/mdec")
COPTS+=		-fstack-protector -Wstack-protector --param ssp-buffer-size=1
.endif

.endif

.if defined(MKSOFTFLOAT) && (${MKSOFTFLOAT} != "no")
COPTS+=		-msoft-float
FOPTS+=		-msoft-float
.endif

.if defined(MKIEEEFP) && (${MKIEEEFP} != "no")
.if ${MACHINE_ARCH} == "alpha"
CFLAGS+=	-mieee
FFLAGS+=	-mieee
.endif
.endif

.if ${MACHINE} == "sparc64" && ${MACHINE_ARCH} == "sparc"
CFLAGS+=	-Wa,-Av8plus
.endif

.if ${MACHINE_ARCH} == "ns32k"
CFLAGS+=	-Wno-uninitialized
.endif

CFLAGS+=	${CPUFLAGS}
AFLAGS+=	${CPUFLAGS}

# Helpers for cross-compiling
HOST_CC?=	cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_COMPILE.cc?=      ${HOST_CXX} ${HOST_CXXFLAGS} ${HOST_CPPFLAGS} -c
.if defined(HOSTPROG_CXX) 
HOST_LINK.c?=	${HOST_CXX} ${HOST_CXXFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}
.else
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}
.endif

HOST_CXX?=	c++
HOST_CXXFLAGS?=	-O

HOST_CPP?=	cpp
HOST_CPPFLAGS?=

HOST_LD?=	ld
HOST_LDFLAGS?=

HOST_AR?=	ar
HOST_RANLIB?=	ranlib

HOST_LN?=	ln

.if !empty(HOST_CYGWIN)
HOST_SH?=	/usr/bin/bash
.else
HOST_SH?=	sh
.endif

ELF2ECOFF?=	elf2ecoff
MKDEP?=		mkdep
OBJCOPY?=	objcopy
OBJDUMP?=	objdump
PAXCTL?=	paxctl
STRIP?=		strip

AWK?=		awk

TOOL_ASN1_COMPILE?=	asn1_compile
TOOL_CAP_MKDB?=		cap_mkdb
TOOL_CAT?=		cat
TOOL_CKSUM?=		cksum
TOOL_COMPILE_ET?=	compile_et
TOOL_CONFIG?=		config
TOOL_CRUNCHGEN?=	crunchgen
TOOL_CTAGS?=		ctags
TOOL_DB?=		db
TOOL_EQN?=		eqn
TOOL_FGEN?=		fgen
TOOL_GENASSYM?=		genassym
TOOL_GENCAT?=		gencat
TOOL_GROFF?=		groff
TOOL_HEXDUMP?=		hexdump
TOOL_INDXBIB?=		indxbib
TOOL_INSTALLBOOT?=	installboot
TOOL_INSTALL_INFO?=	install-info
TOOL_JOIN?=		join
TOOL_M4?=		m4
TOOL_MAKEFS?=		makefs
TOOL_MAKEINFO?=		makeinfo
TOOL_MAKEWHATIS?=	/usr/libexec/makewhatis
TOOL_MDSETIMAGE?=	mdsetimage
TOOL_MENUC?=		menuc
TOOL_MKCSMAPPER?=	mkcsmapper
TOOL_MKESDB?=		mkesdb
TOOL_MKLOCALE?=		mklocale
TOOL_MKMAGIC?=		file
TOOL_MKTEMP?=		mktemp
TOOL_MSGC?=		msgc
TOOL_MTREE?=		mtree
TOOL_PAX?=		pax
TOOL_PIC?=		pic
TOOL_PREPMKBOOTIMAGE?=	prep-mkbootimage
TOOL_PWD_MKDB?=		pwd_mkdb
TOOL_REFER?=		refer
TOOL_ROFF_ASCII?=	nroff
TOOL_ROFF_DVI?=		${TOOL_GROFF} -Tdvi
TOOL_ROFF_HTML?=	${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS?=		${TOOL_GROFF} -Tps
TOOL_ROFF_RAW?=		${TOOL_GROFF} -Z
TOOL_RPCGEN?=		rpcgen
TOOL_SED?=		sed
TOOL_SOELIM?=		soelim
TOOL_STAT?=		stat
TOOL_SPARKCRC?=		sparkcrc
TOOL_SUNLABEL?=		sunlabel
TOOL_TBL?=		tbl
TOOL_UUDECODE?=		uudecode
TOOL_VGRIND?=		vgrind -f
TOOL_ZIC?=		zic

.SUFFIXES:	.o .ln .lo .c .cc .cpp .cxx .C .m ${YHEADER:D.h}

# C
.c.o:
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.c.ln:
	${_MKTARGET_COMPILE}
	${LINT} ${LINTFLAGS} \
	    ${CPPFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS.${.IMPSRC:T}:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    -i ${.IMPSRC}

# C++
.cc.o .cpp.o .cxx.o .C.o:
	${_MKTARGET_COMPILE}
	${COMPILE.cc} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.m.o:
	${_MKTARGET_COMPILE}
	${COMPILE.m} ${OBJCOPTS} ${OBJCOPTS.${.IMPSRC:T}} ${.IMPSRC}

# Host-compiled C objects
# The intermediate step is necessary for Sun CC, which objects to calling
# object files anything but *.o
.c.lo:
	${_MKTARGET_COMPILE}
	${HOST_COMPILE.c} -o ${.TARGET}.o ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
	mv ${.TARGET}.o ${.TARGET}

# C++
.cc.lo .cpp.lo .cxx.lo .C.lo:
	${_MKTARGET_COMPILE}
	${HOST_COMPILE.cc} -o ${.TARGET}.o ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
	mv ${.TARGET}.o ${.TARGET}

# Assembly
.s.o:
	${_MKTARGET_COMPILE}
	${COMPILE.s} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

.S.o:
	${_MKTARGET_COMPILE}
	${COMPILE.S} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

# Lex
LPREFIX?=	yy
LFLAGS+=	-P${LPREFIX}

.l.c:
	${_MKTARGET_LEX}
	${LEX.l} -o${.TARGET} ${.IMPSRC}

# Yacc
YFLAGS+=	${YPREFIX:D-p${YPREFIX}} ${YHEADER:D-d}

.y.c:
	${_MKTARGET_YACC}
	${YACC.y} -o ${.TARGET} ${.IMPSRC}

.ifdef YHEADER
.y.h: ${.TARGET:.h=.c}
.endif

.endif	# !defined(_BSD_SYS_MK_)
