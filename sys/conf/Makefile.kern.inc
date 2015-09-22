#	$NetBSD: Makefile.kern.inc,v 1.183.2.2 2015/09/22 12:05:56 skrll Exp $
#
# This file contains common `MI' targets and definitions and it is included
# at the bottom of each `MD' ${MACHINE}/conf/Makefile.${MACHINE}.
#
# Each target in this file should be protected with `if !target(target)'
# or `if !commands(target)' and each variable should only be conditionally
# assigned `VAR ?= VALUE', so that everything can be overriden.
#
# DEBUG is set to -g if debugging.
# PROF is set to -pg if profiling.
#
# To specify debugging, add the config line: makeoptions DEBUG="-g"
# A better way is to specify -g only for a few files.
#
#	makeoptions DEBUGLIST="uvm* trap if_*"
#
# all ports are expected to include bsd.own.mk for toolchain settings

# Default DEBUG to -g if kernel debug info is requested by MKKDEBUG=yes
.if defined(MKKDEBUG) && ${MKKDEBUG} == "yes"
DEBUG?=-g
.endif

##
## (0) toolchain settings for things that aren't part of the standard
## toolchain
##
HOST_SH?=	sh
DBSYM?=		dbsym
MKDEP?=		mkdep
STRIP?=		strip
OBJCOPY?=	objcopy
OBJDUMP?=	objdump
CSCOPE?=	cscope
MKID?=		mkid
UUDECODE?=	${TOOL_UUDECODE:Uuudecode}
HEXDUMP?=	${TOOL_HEXDUMP:Uhexdump}
GENASSYM?=	${TOOL_GENASSYM:Ugenassym}
.MAKEOVERRIDES+=USETOOLS	# make sure proper value is propagated

_MKMSG?=		@\#
_MKSHMSG?=		echo
_MKSHECHO?=		echo
_MKSHNOECHO=		:
_MKMSG_CREATE?=		:
_MKTARGET_COMPILE?=	:
_MKTARGET_CREATE?=	:

##
## (1) port independent source tree identification
##
# source tree is located via $S relative to the compilation directory
.ifndef S
S!=	cd ../../../..; pwd
.endif

##
## (2) compile settings
##
## CPPFLAGS, CFLAGS, and AFLAGS must be set in the port's Makefile
##
INCLUDES?=	-I. ${EXTRA_INCLUDES} -I${S}/../common/include -I$S/arch \
		-I$S -nostdinc
CPPFLAGS+=	${INCLUDES} ${IDENT} -D_KERNEL -D_KERNEL_OPT
CPPFLAGS+=	-std=gnu99
DEFCOPTS?=	-O2
COPTS?=		${DEFCOPTS}
DBG=		# might contain unwanted -Ofoo
CWARNFLAGS+=	-Wall -Wno-main -Wno-format-zero-length -Wpointer-arith
CWARNFLAGS+=	-Wmissing-prototypes -Wstrict-prototypes
CWARNFLAGS+=	-Wold-style-definition
CWARNFLAGS+=	-Wswitch -Wshadow
CWARNFLAGS+=	-Wcast-qual -Wwrite-strings
CWARNFLAGS+=	-Wno-unreachable-code
#CWARNFLAGS+=	-Wc++-compat -Wno-error=c++-compat
CWARNFLAGS+=	-Wno-pointer-sign -Wno-attributes
.  if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
	${MACHINE_ARCH} == "sparc64" || ${MACHINE} == "prep"
CWARNFLAGS+=	-Wextra -Wno-unused-parameter
.  endif
.  if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
CWARNFLAGS+=	-Wold-style-definition
.  endif
# Add -Wno-sign-compare.  -Wsign-compare is included in -Wall as of GCC 3.3,
# but our sources aren't up for it yet.
CWARNFLAGS+=	-Wno-sign-compare

CWARNFLAGS.clang+=	-Wno-unknown-pragmas -Wno-conversion \
			-Wno-self-assign

CWARNFLAGS.ah_regdomain.c= ${${ACTIVE_CC} == "clang":? \
    -Wno-shift-count-negative -Wno-shift-count-overflow:}

CWARNFLAGS.ioconf.c= ${${ACTIVE_CC} == "clang":? -Wno-unused-const-variable :}

CFLAGS+=	-ffreestanding -fno-zero-initialized-in-bss
CFLAGS+=	${DEBUG} ${COPTS}
AFLAGS+=	-D_LOCORE -Wa,--fatal-warnings

# XXX
.if defined(HAVE_GCC) || defined(HAVE_LLVM)
CFLAGS+=	-fno-strict-aliasing
CFLAGS+=	-fno-common
.endif

# Use the per-source COPTS variables to add -g to just those
# files that match the shell patterns given in ${DEBUGLIST}
#
.for i in ${DEBUGLIST}
. for j in ${ALLFILES:M*.c:T:M$i.c}
COPTS.${j}+=-g
. endfor
.endfor

# Always compile debugsyms.c with debug information.
# This allows gdb to use type informations.
#
COPTS.debugsyms.c+=	-g

# Add CTF sections for DTrace
.if defined(CTFCONVERT)
COMPILE_CTFCONVERT=	${_MKSHECHO}\
			${CTFCONVERT} ${CTFFLAGS} ${.TARGET} && \
			${CTFCONVERT} ${CTFFLAGS} ${.TARGET}
.else
COMPILE_CTFCONVERT=	${_MKSHNOECHO}
.endif

KCOMPILE.c=	${CC} ${COPTS.${<:T}} ${CFLAGS} ${CPPFLAGS} -c $< -o $@
KCOMPILE.s=	${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${CPPFLAGS} -c $< -o $@
KLINK.o=	${LD} -r ${LINKFORMAT} -Map=${.TARGET}.map -o ${.TARGET} ${.ALLSRC}

# compile rules: rules are named ${TYPE}_${SUFFIX} where TYPE is NORMAL or
# NOPROF and SUFFIX is the file suffix, capitalized (e.g. C for a .c file).
NORMAL_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO} ${KCOMPILE.c} ${PROF} && \
		${KCOMPILE.c} ${PROF} && \
		${COMPILE_CTFCONVERT}
NOPROF_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO} ${KCOMPILE.c} && \
		${KCOMPILE.c} && \
		${COMPILE_CTFCONVERT}
NORMAL_S?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO} ${KCOMPILE.s} && \
		${KCOMPILE.s}

# link rules:
LINK_O?=	@${_MKSHMSG} "   link  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO} ${KLINK.o} && \
		${KLINK.o}

##
## (3) libkern and compat
##
## Set KERN_AS in the port Makefile to "obj" or "library".  The
## default is "library", as documented in $S/lib/libkern/Makefile.inc.
##

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
.ifndef PROF
LIBKERN?=	${KERNLIB}
.else
LIBKERN?=	${KERNLIB_PROF}
.endif

LIBKERNLN?=	${KERNLIBLN}

### find out what to use for libcompat
.include "$S/compat/common/Makefile.inc"
.ifndef PROF
SYSLIBCOMPAT?=	${COMPATLIB}
.else
SYSLIBCOMPAT?=	${COMPATLIB_PROF}
.endif

SYSLIBCOMPATLN?=	${COMPATLIBLN}

##
## (4) local objects, compile rules, and dependencies
##
## Each port should have a corresponding section with settings for
## MD_CFILES, MD_SFILES, and MD_OBJS, along with build rules for same.
##

.if !defined(___USE_SUFFIX_RULES___)
_MD_OBJS=	${MD_OBJS:T}
.else
_MD_OBJS=	${MD_OBJS}
.endif

##
## (5) link settings
##
## TEXTADDR (or LOADADDRESS), LINKFORMAT, LINKSCRIPT, and any EXTRA_LINKFLAGS
## must be set in the port's Makefile.  The port specific definitions for
## LINKFLAGS_NORMAL and LINKFLAGS_DEBUG will added to the LINKFLAGS
## depending on the value of DEBUG.
##
# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxxx.o vers.o build_kernel

SYSTEM_LIB=	${MD_LIBS} ${SYSLIBCOMPAT} ${LIBKERN}
SYSTEM_OBJ?=	${_MD_OBJS} ${OBJS} ${SYSTEM_LIB}
SYSTEM_DEP+=	Makefile ${SYSTEM_OBJ:O}
.if defined(CTFMERGE)
SYSTEM_CTFMERGE= ${CTFMERGE} ${CTFMFLAGS} -o ${.TARGET} ${SYSTEM_OBJ} ${EXTRA_OBJ} vers.o
.else
SYSTEM_CTFMERGE= ${_MKSHECHO}
.endif
SYSTEM_LD_HEAD?=@rm -f $@
SYSTEM_LD?=	${_MKSHMSG} "   link  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${LD} -Map ${.TARGET}.map --cref ${LINKFLAGS} -o ${.TARGET} \
		'$${SYSTEM_OBJ:N*swap*netbsd*}' '$${EXTRA_OBJ}' vers.o \
		${OBJS:M*swap${.TARGET}.o}; \
		${LD} -Map ${.TARGET}.map --cref ${LINKFLAGS} -o ${.TARGET} \
		${SYSTEM_OBJ:N*swap*netbsd*} ${EXTRA_OBJ} vers.o \
		${OBJS:M*swap${.TARGET}.o}

TEXTADDR?=	${LOADADDRESS}			# backwards compatibility
LINKTEXT?=	${TEXTADDR:C/.+/-Ttext &/}
LINKDATA?=	${DATAADDR:C/.+/-Tdata &/}
ENTRYPOINT?=	start
LINKENTRY?=	${ENTRYPOINT:C/.+/-e &/}
LINKFLAGS?=	${LINKFORMAT} ${LINKSCRIPT} ${LINKTEXT} ${LINKDATA} ${LINKENTRY} \
		${EXTRA_LINKFLAGS}

LINKFLAGS_DEBUG?=	-X

SYSTEM_LD_TAIL?=@${TOOL_SED} '/const char sccs/!d;s/.*@(.)//;s/" "//;s/\\.*//' vers.c; \
		${SIZE} $@; chmod 755 $@; \
		${SYSTEM_CTFMERGE}
SYSTEM_LD_TAIL_DEBUG?=; \
		echo mv -f $@ $@.gdb; mv -f $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} -o $@ $@.gdb; \
		${STRIP} ${STRIPFLAGS} -o $@ $@.gdb
LINKFLAGS_NORMAL?=	-S
STRIPFLAGS?=	-g

DEBUG?=
.if !empty(DEBUG:M-g*)
SYSTEM_LD_TAIL+=${SYSTEM_LD_TAIL_DEBUG}
LINKFLAGS+=	${LINKFLAGS_DEBUG}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.gdb@}
CTFFLAGS+=	-g
TARGETSFX=	.gdb
.elifndef PROF
LINKFLAGS+=	${LINKFLAGS_NORMAL}
.endif

SYSTEM_LD_HEAD+=	${SYSTEM_LD_HEAD_EXTRA}
SYSTEM_LD_TAIL_STAGE1=	${SYSTEM_LD_TAIL}
SYSTEM_LD_TAIL_STAGE2=	${SYSTEM_LD_TAIL}
.if defined(COPY_SYMTAB)
SYSTEM_LD_TAIL_STAGE2+=	; echo ${DBSYM} $@; ${DBSYM} $@
.if !empty(DEBUG:M-g)
SYSTEM_LD_TAIL_STAGE2+=	; echo ${DBSYM} $@.gdb; ${DBSYM} $@.gdb
.endif
.endif
SYSTEM_LD_TAIL_STAGE2+=	${SYSTEM_LD_TAIL_EXTRA}

##
## (6) port independent targets and dependencies: assym.h, vers.o
##

.if !defined(___USE_SUFFIX_RULES___)

# Generate list of *.o files to pass to ${LD}, preserving order.
#	x/y/z/a.[csS] -> a.[csS]
#	a.[csS] -> a.o
OBJS=	${ALLFILES:C|^.*/([^/]*\.[csS])$$|\1|:C|^(.*)\.[csS]$$|\1.o|}

CFILES=	${ALLFILES:M*.c}
SFILES=	${ALLFILES:M*.[sS]}
OFILES=	${ALLFILES:M*.o}
# absolute, generated (build directory), relative (under $S)
_CFILES=${CFILES:M/*} ${CFILES:N/*:N*/*} ${CFILES:N/*:M*/*:C|^|$S/|}
_SFILES=${SFILES:M/*} ${SFILES:N/*:N*/*} ${SFILES:N/*:M*/*:C|^|$S/|}
_MD_CFILES=${MD_CFILES}
_MD_SFILES=${MD_SFILES}
CSRCS=	${_MD_CFILES} ${_CFILES}
SSRCS=	${_MD_SFILES} ${_SFILES}
SRCS=	${CSRCS} ${SSRCS}

.else # ___USE_SUFFIX_RULES___
OBJS=	${ALLFILES:C|\.[csS]$$|.o|}
SRCS=	${ALLFILES:M*.[csS]}
.endif # ___USE_SUFFIX_RULES___

.if !defined(___USE_SUFFIX_RULES___)
.for _s in ${_CFILES}
.if !commands(${_s:T:R}.o)
${_s:T:R}.o: ${_s}
	${NORMAL_C}
.endif
.endfor

.for _s in ${_SFILES}
.if !commands(${_s:T:R}.o)
${_s:T:R}.o: ${_s}
	${NORMAL_S}
.endif
.endfor
.endif # !___USE_SUFFIX_RULES___

##
## (7) misc targets: install, clean(dir), depend(all), lint, links, tags,
##                   cscope, mkid
##
## Any ports that have other stuff to be cleaned up should fill in
## EXTRA_CLEAN.  Some ports may want different settings for
## KERNLINTFLAGS, MKDEP_CFLAGS, or MKDEP_AFLAGS.
##

##
## clean
##

.if !target(__CLEANKERNEL)
__CLEANKERNEL: .USE
	${_MKMSG} "${.TARGET}ing the kernel objects"
	rm -f ${KERNELS} *.map eddep tags *.[io] *.ko *.ln [a-z]*.s vers.c \
	    [Ee]rrs linterrs makelinks assym.h.tmp assym.h \
	    ${EXTRA_KERNELS} ${EXTRA_CLEAN}
.endif

.if !target(kernelnames)
kernelnames:
	@echo "${KERNELS} ${EXTRA_KERNELS}"
.endif

.if !target(__CLEANDEPEND)
__CLEANDEPEND: .USE
	echo .depend ${DEPS} | xargs rm -f --
.endif

# do not !target these, the kern and compat Makefiles augment them
cleandir distclean: __CLEANKERNEL __CLEANDEPEND
clean: __CLEANKERNEL
depend: .depend
dependall: depend .WAIT all

##
## depend
##

.if !target(.depend)
MKDEP_AFLAGS?=	${AFLAGS}
MKDEP_CFLAGS?=	${CFLAGS}
.if !defined(___USE_SUFFIX_RULES___)
DEPS=	${SRCS:T:R:S/$/.d/g}
.else
DEPS=	${SRCS:R:S/$/.d/g}
.endif

.if !defined(___USE_SUFFIX_RULES___)
.for _s in ${SSRCS}
.if !target(${_s:T:R}.d)
${_s:T:R}.d: ${_s}
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET}.tmp -- ${MKDEP_AFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
	mv -f ${.TARGET}.tmp ${.TARGET}
.endif
.endfor

.for _s in ${CSRCS}
.if !target(${_s:T:R}.d)
${_s:T:R}.d: ${_s}
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET}.tmp -- ${MKDEP_CFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
	mv -f ${.TARGET}.tmp ${.TARGET}
.endif
.endfor
.endif # !___USE_SUFFIX_RULES___

.depend: ${DEPS:O}
	${_MKTARGET_CREATE}
	echo "${.ALLSRC}" | ${MKDEP} -D
.endif

##
## install
##

# List of kernel images that will be installed into the root file system.
# Some platforms may need to install more than one (e.g. a netbsd.aout file
# to be loaded directly by the firmware), so this can be overriden by them.
KERNIMAGES?=	netbsd

.if !target(install)
# The install target can be redefined by putting a
# install-kernel-${MACHINE_NAME} target into /etc/mk.conf
MACHINE_NAME!=  uname -n
install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME})
install-kernel-${MACHINE_NAME}:
.for _K in ${KERNIMAGES}
	rm -f ${DESTDIR}/o${_K}
	ln ${DESTDIR}/${_K} ${DESTDIR}/o${_K}
	cp ${_K} ${DESTDIR}/n${_K}
	mv ${DESTDIR}/n${_K} ${DESTDIR}/${_K}
.endfor
.endif
.endif

.include "${S}/conf/ldscript.mk"
.include "${S}/conf/assym.mk"
.include "${S}/conf/newvers.mk"
.include "${S}/dev/splash/splash.mk"
.include "${S}/conf/mdroot.mk"
.include "${S}/conf/lint.mk"
.include "${S}/conf/cscope.mk"
.include "${S}/conf/gdbinit.mk"
.include "${S}/conf/ssp.mk"

##
## the kernel
##

AFLAGS+=	${AOPTS.${.IMPSRC:T}}
CFLAGS+=	${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}}
CPPFLAGS+=	${CPPFLAGS.${.IMPSRC:T}}
CWARNFLAGS+=	${CWARNFLAGS.${.IMPSRC:T}}

.MAIN: all
all: .gdbinit
.for k in ${KERNELS}
all: .WAIT ${k}
${k}: ${SYSTEM_DEP:O} swap${k}.o vers.o build_kernel
.endfor

.if !defined(COPY_SYMTAB)
build_kernel: .USE
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD}
	${SYSTEM_LD_TAIL_STAGE2}
.else
.for k in ${KERNELS}
${k}: $S/kern/kern_ksyms_buf.c
.endfor
build_kernel: .USE
	${CC} ${CFLAGS} ${CPPFLAGS} \
	    -c $S/kern/kern_ksyms_buf.c -o kern_ksyms_buf.o
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} kern_ksyms_buf.o
	${SYSTEM_LD_TAIL_STAGE1}
	${CC} ${CFLAGS} ${CPPFLAGS} \
	    -DSYMTAB_SPACE=$$(${DBSYM} -P ${.TARGET}${TARGETSFX}) \
	    -c $S/kern/kern_ksyms_buf.c -o kern_ksyms_buf_real.o
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} kern_ksyms_buf_real.o
	${SYSTEM_LD_TAIL_STAGE2}
.endif

.include <bsd.files.mk>
.include <bsd.clang-analyze.mk>

##
## suffix rules
##

.if defined(___USE_SUFFIX_RULES___)
.SUFFIXES: .s .d
.s.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f $@.tmp -- ${MKDEP_AFLAGS} ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<
	mv -f $@.tmp $@

.SUFFIXES: .S .d
.S.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f $@.tmp -- ${MKDEP_AFLAGS} ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<
	mv -f $@.tmp $@

.SUFFIXES: .c .d
.c.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f $@.tmp -- ${MKDEP_CFLAGS} ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<
	mv -f $@.tmp $@

.SUFFIXES: .c .o .go .po
.c.o:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.c}
	@${KCOMPILE.c}
	@${COMPILE_CTFCONVERT}
.c.go:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.c} -g
	@${KCOMPILE.c} -g
	@${COMPILE_CTFCONVERT}
.c.po:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.c} -pg
	@${KCOMPILE.c} -pg
	@${COMPILE_CTFCONVERT}

.SUFFIXES: .s .o .go .po
.s.o:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s}
	@${KCOMPILE.s}
.s.go:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s} -g
	@${KCOMPILE.s} -g
.s.po:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s} -pg
	@${KCOMPILE.s} -pg

.S.o:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s}
	@${KCOMPILE.s}
.S.go:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s} -g
	@${KCOMPILE.s} -g
.S.po:
	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}"
	@${_MKSHECHO} ${KCOMPILE.s} -pg
	@${KCOMPILE.s} -pg
.endif # ___USE_SUFFIX_RULES___

##
## the end
##
