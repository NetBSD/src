#	$NetBSD: bsd.sys.mk,v 1.102 2003/10/19 14:23:02 lukem Exp $
#
# Build definitions used for NetBSD source tree builds.

.if !defined(_BSD_SYS_MK_)
_BSD_SYS_MK_=1

MAKEVERBOSE?=	2

.if ${MAKEVERBOSE} == 0
_MKMSG=		@\#
_MKCMD=		@
_MKSHMSG=	: echo
_MKSHECHO=	: echo
.elif ${MAKEVERBOSE} == 1
_MKMSG=		@echo '   '
_MKCMD=		@
_MKSHMSG=	echo '   '
_MKSHECHO=	: echo
.else	# MAKEVERBOSE == 2 ?
_MKMSG=		@echo '\#  '
_MKCMD=	
_MKSHMSG=	echo '\#  '
_MKSHECHO=	echo
.endif
_MKMSGBUILD=	${_MKMSG} "  build  ${.TARGET}"
_MKMSGCREATE=	${_MKMSG} " create  ${.TARGET}"
_MKMSGCREATE.m=	${_MKMSG} " create "
_MKMSGCOMPILE=	${_MKMSG} "compile  ${.TARGET}"
_MKMSGFORMAT=	${_MKMSG} " format  ${.TARGET}"
_MKMSGINSTALL=	${_MKMSG} "install  ${.TARGET}"
_MKMSGINSTALL.m=${_MKMSG} "install "
_MKMSGLINK=	${_MKMSG} "   link  ${.TARGET}"
_MKMSGLINK.m=	${_MKMSG} "   link "
_MKMSGLEX=	${_MKMSG} "    lex  ${.TARGET}"
_MKMSGYACC=	${_MKMSG} "   yacc  ${.TARGET}"

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
# XXX Delete -Wuninitialized by default for now -- the compiler doesn't
# XXX always get it right.
CFLAGS+=	-Wno-uninitialized
.endif
.if ${WARNS} > 1
CFLAGS+=	-Wreturn-type -Wswitch -Wshadow
.endif
.if ${WARNS} > 2
CFLAGS+=	-Wcast-qual -Wwrite-strings
.endif
.endif

.if defined(WFORMAT) && defined(FORMAT_AUDIT)
.if ${WFORMAT} > 1
CFLAGS+=	-Wnetbsd-format-audit -Wno-format-extra-args
.endif
.endif

CPPFLAGS+=	${AUDIT:D-D__AUDIT__}
CFLAGS+=	${CWARNFLAGS} ${NOGCCERROR:D:U-Werror}
LINTFLAGS+=	${DESTDIR:D-d ${DESTDIR}/usr/include}

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

CFLAGS+=	${CPUFLAGS}

# Helpers for cross-compiling
HOST_CC?=	cc
HOST_CFLAGS?=	-O
HOST_COMPILE.c?=${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} -c
HOST_LINK.c?=	${HOST_CC} ${HOST_CFLAGS} ${HOST_CPPFLAGS} ${HOST_LDFLAGS}

HOST_CXX?=	c++
HOST_CXXFLAGS?=	-O

HOST_CPP?=	cpp
HOST_CPPFLAGS?=

HOST_LD?=	ld
HOST_LDFLAGS?=

HOST_AR?=	ar
HOST_RANLIB?=	ranlib

.if !empty(HOST_CYGWIN)
HOST_SH?=	/usr/bin/bash
.else
HOST_SH?=	sh
.endif

ELF2ECOFF?=	elf2ecoff
MKDEP?=		mkdep
OBJCOPY?=	objcopy
STRIP?=		strip

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
TOOL_GENCAT?=		gencat
TOOL_GROFF?=		groff
TOOL_HEXDUMP?=		hexdump
TOOL_INDXBIB?=		indxbib
TOOL_INSTALLBOOT?=	installboot
TOOL_INSTALL_INFO?=	install-info
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
TOOL_SOELIM?=		soelim
TOOL_STAT?=		stat
TOOL_SUNLABEL?=		sunlabel
TOOL_TBL?=		tbl
TOOL_UUDECODE?=		uudecode
TOOL_VGRIND?=		vgrind -f
TOOL_ZIC?=		zic

.SUFFIXES:	.c .m .o .ln .lo .s .S .l .y ${YHEADER:D.h}

# C
.c:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${LINK.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.c.o:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${COMPILE.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
.c.ln:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${LINT} ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} ${CPPFLAGS.${.IMPSRC:T}:M-[IDU]*} -i ${.IMPSRC}

# Objective C
# (Defined here rather than in <sys.mk> because `.m' is not just
#  used for Objective C source)
.m:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${LINK.m} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.m.o:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${COMPILE.m} ${.IMPSRC}

# Host-compiled C objects
# The intermediate step is necessary for Sun CC, which objects to calling
# object files anything but *.o
.c.lo:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${HOST_COMPILE.c} -o ${.TARGET}.o ${.IMPSRC}
	${_MKCMD}\
	mv ${.TARGET}.o ${.TARGET}

# Assembly
.s:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${LINK.s} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.s.o:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${COMPILE.s} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
.S:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${LINK.S} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} -o ${.TARGET} ${.IMPSRC} ${LDLIBS}
.S.o:
	${_MKMSGCOMPILE}
	${_MKCMD}\
	${COMPILE.S} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}

# Lex
LPREFIX?=	yy
LFLAGS+=	-P${LPREFIX}

.l.o: # remove to force use of .l.c->.c.o transforms
.l:
	${_MKMSGLEX}
	${_MKCMD}\
	${LEX.l} -o${.TARGET:R}.${LPREFIX}.c ${.IMPSRC}
	${_MKCMD}\
	${LINK.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}}-o ${.TARGET} ${.TARGET:R}.${LPREFIX}.c ${LDLIBS} -ll
	${_MKCMD}\
	rm -f ${.TARGET:R}.${LPREFIX}.c
.l.c:
	${_MKMSGLEX}
	${_MKCMD}\
	${LEX.l} -o${.TARGET} ${.IMPSRC}

# Yacc
YFLAGS+=	${YPREFIX:D-p${YPREFIX}} ${YHEADER:D-d}

.y.o: # remove to force use of .y.c->.c.o transforms
.y:
	${_MKMSGYACC}
	${_MKCMD}\
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${_MKCMD}\
	${LINK.c} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}}-o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	${_MKCMD}\
	rm -f ${.TARGET:R}.tab.[ch]
.y.c:
	${_MKMSGYACC}
	${_MKCMD}\
	${YACC.y} -o ${.TARGET} ${.IMPSRC}

.ifdef YHEADER
.y.h: ${.TARGET:.h=.c}
.endif

.endif	# !defined(_BSD_SYS_MK_)
