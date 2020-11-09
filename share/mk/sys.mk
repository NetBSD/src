#	$NetBSD: sys.mk,v 1.144 2020/11/09 16:15:05 christos Exp $
#	@(#)sys.mk	8.2 (Berkeley) 3/21/94
#
# This file contains the basic rules for make(1) and is read first
# Do not put conditionals that are set on different files here and
# expect them to work.

unix?=		We run NetBSD.

.SUFFIXES: .a .o .ln .s .S .c .cc .cpp .cxx .C .f .F .r .p .l .y .sh

.LIBS:		.a

AR?=		ar
ARFLAGS?=	rl
RANLIB?=	ranlib
MV?=		mv -f

AS?=		as
AFLAGS?=
COMPILE.s?=	${CC} ${AFLAGS} ${AFLAGS.${<:T}} -c
LINK.s?=	${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${LDFLAGS}
_ASM_TRADITIONAL_CPP=	-x assembler-with-cpp
COMPILE.S?=	${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${CPPFLAGS} ${_ASM_TRADITIONAL_CPP} -c
LINK.S?=	${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${CPPFLAGS} ${LDFLAGS}

CC?=		cc
.if ${MACHINE_ARCH} == "sh3el" || ${MACHINE_ARCH} == "sh3eb"
# -O2 is too -falign-* zealous for low-memory sh3 machines
DBG?=	-Os -freorder-blocks
.elif ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "m68000"
# -freorder-blocks (enabled by -O2) produces much bigger code
DBG?=	-O2 -fno-reorder-blocks
.elif ${MACHINE_ARCH} == "coldfire"
DBG?=	-O1
.else
DBG?=	-O2
.endif
.if ${MKDTRACE:Uno} != "no"
DTRACE_OPTS?=	-fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-ipa-sra -fno-ipa-icf
.endif
CFLAGS?=	${DBG}
LDFLAGS?=
COMPILE.c?=	${CC} ${CFLAGS} ${DTRACE_OPTS} ${CPPFLAGS} -c
LINK.c?=	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS}

# C Type Format data is required for DTrace
CTFFLAGS	?=	-g -L VERSION
CTFMFLAGS	?=	-t -g -L VERSION
OBJECT_TARGET	?= -o ${.TARGET}${defined(CTFCONVERT):?.o:}
CTFCONVERT_RUN	?= ${defined(CTFCONVERT):?${CTFCONVERT} ${CTFFLAGS} -o ${.TARGET} ${.TARGET}.o && rm -f ${.TARGET}.o:}

CXX?=		c++
# Strip flags unsupported by C++ compilers
# Remove -Wsystem-headers because C++ headers aren't clean of warnings
CXXFLAGS?=	${CFLAGS:N-Wno-traditional:N-Wstrict-prototypes:N-Wmissing-prototypes:N-Wno-pointer-sign:N-ffreestanding:N-std=gnu[0-9][0-9]:N-Wold-style-definition:N-Wno-format-zero-length:N-Wsystem-headers}

# Use the sources, as the seed... Normalize all paths...
__ALLSRC1=	${empty(DESTDIR):?${.ALLSRC}:${.ALLSRC:S|^${DESTDIR}|^destdir|}}
__ALLSRC2=	${empty(MAKEOBJDIR):?${__ALLSRC1}:${__ALLSRC1:S|^${MAKEOBJDIR}|^obj|}}
__ALLSRC3=	${empty(NETBSDSRCDIR):?${__ALLSRC2}:${__ALLSRC2:S|^${NETBSDSRCDIR}|^src|}}
__ALLSRC4=	${empty(X11SRCDIR):?${__ALLSRC3}:${__ALLSRC3:S|^${X11SRCDIR}|^xsrc|}}
# Skip paths that contain relative components and can't be normalized, sort..
__INITSEED=	${__ALLSRC4:N*/../*:O}

__BUILDSEED=	${BUILDSEED}/${__INITSEED}/${.TARGET}
_CXXSEED?=	${BUILDSEED:D-frandom-seed=${__BUILDSEED:hash}}

COMPILE.cc?=	${CXX} ${_CXXSEED} ${CXXFLAGS} ${DTRACE_OPTS} ${CPPFLAGS} -c
LINK.cc?=	${CXX} ${CXXFLAGS} ${CPPFLAGS} ${LDFLAGS}

OBJC?=		${CC}
OBJCFLAGS?=	${CFLAGS}
COMPILE.m?=	${OBJC} ${OBJCFLAGS} ${CPPFLAGS} -c
LINK.m?=	${OBJC} ${OBJCFLAGS} ${CPPFLAGS} ${LDFLAGS}

CPP?=		cpp
CPPFLAGS?=

FC?=		f77
FFLAGS?=	-O
RFLAGS?=
COMPILE.f?=	${FC} ${FFLAGS} -c
LINK.f?=	${FC} ${FFLAGS} ${LDFLAGS}
COMPILE.F?=	${FC} ${FFLAGS} ${CPPFLAGS} -c
LINK.F?=	${FC} ${FFLAGS} ${CPPFLAGS} ${LDFLAGS}
COMPILE.r?=	${FC} ${FFLAGS} ${RFLAGS} -c
LINK.r?=	${FC} ${FFLAGS} ${RFLAGS} ${LDFLAGS}

INSTALL?=	install

LD?=		ld

LEX?=		lex
LFLAGS?=
LEX.l?=		${LEX} ${LFLAGS}

LINT?=		lint
LINTFLAGS?=	-chapbrxzgFS

LORDER?=	lorder

MAKE?=		make

NM?=		nm

PC?=		pc
PFLAGS?=
COMPILE.p?=	${PC} ${PFLAGS} ${CPPFLAGS} -c
LINK.p?=	${PC} ${PFLAGS} ${CPPFLAGS} ${LDFLAGS}

SHELL?=		sh

SIZE?=		size

TSORT?= 	tsort -q

YACC?=		yacc
YFLAGS?=
YACC.y?=	${YACC} ${YFLAGS}

# C
.c:
	${LINK.c} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
# XXX: disable for now
#	${CTFCONVERT_RUN}
.c.o:
	${COMPILE.c} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.c.a:
	${COMPILE.c} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o
.c.ln:
	${LINT} ${LINTFLAGS} \
	    ${CPPFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    -i ${.IMPSRC}

# C++
.cc .cpp .cxx .C:
	${LINK.cc} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
# XXX: disable for now
#	${CTFCONVERT_RUN}
.cc.o .cpp.o .cxx.o .C.o:
	${COMPILE.cc} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.cc.a .cpp.a .cxx.a .C.a:
	${COMPILE.cc} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

# Fortran/Ratfor
.f:
	${LINK.f} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.f.o:
	${COMPILE.f} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.f.a:
	${COMPILE.f} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

.F:
	${LINK.F} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.F.o:
	${COMPILE.F} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.F.a:
	${COMPILE.F} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

.r:
	${LINK.r} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.r.o:
	${COMPILE.r} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.r.a:
	${COMPILE.r} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

# Pascal
.p:
	${LINK.p} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.p.o:
	${COMPILE.p} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.p.a:
	${COMPILE.p} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

# Assembly
.s:
	${LINK.s} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.s.o:
	${COMPILE.s} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.s.a:
	${COMPILE.s} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o
.S:
	${LINK.S} ${OBJECT_TARGET} ${.IMPSRC} ${LDLIBS}
	${CTFCONVERT_RUN}
.S.o:
	${COMPILE.S} ${.IMPSRC} ${OBJECT_TARGET}
	${CTFCONVERT_RUN}
.S.a:
	${COMPILE.S} ${.IMPSRC}
	${AR} ${ARFLAGS} ${.TARGET} ${.PREFIX}.o
	rm -f ${.PREFIX}.o

# Lex
.l:
	${LEX.l} ${.IMPSRC}
	${LINK.c} ${OBJECT_TARGET} lex.yy.c ${LDLIBS} -ll
	${CTFCONVERT_RUN}
	rm -f lex.yy.c
.l.c:
	${LEX.l} ${.IMPSRC}
	${MV} lex.yy.c ${.TARGET}
.l.o:
	${LEX.l} ${.IMPSRC}
	${COMPILE.c} ${OBJECT_TARGET} lex.yy.c
	${CTFCONVERT_RUN}
	rm -f lex.yy.c

# Yacc
.y:
	${YACC.y} ${.IMPSRC}
	${LINK.c} ${OBJECT_TARGET} y.tab.c ${LDLIBS}
	${CTFCONVERT_RUN}
	rm -f y.tab.c
.y.c:
	${YACC.y} ${.IMPSRC}
	${MV} y.tab.c ${.TARGET}
.y.o:
	${YACC.y} ${.IMPSRC}
	${COMPILE.c} ${OBJECT_TARGET} y.tab.c
	${CTFCONVERT_RUN}
	rm -f y.tab.c

# Shell
.sh:
	rm -f ${.TARGET}
	cp ${.IMPSRC} ${.TARGET}
	chmod a+x ${.TARGET}
