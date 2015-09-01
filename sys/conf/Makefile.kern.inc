#	$NetBSD: Makefile.kern.inc,v 1.218 2015/09/01 23:04:35 uebayasi Exp $
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

.if ${USE_SSP:Uno} == "yes"
COPTS.kern_ssp.c+=	-fno-stack-protector -D__SSP__
.endif

# for multi-cpu machines, cpu_hatch() straddles the init of
# __stack_chk_guard, so ensure stack protection is disabled
.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
COPTS.cpu.c+=		-fno-stack-protector
.endif

# Use the per-source COPTS variables to add -g to just those
# files that match the shell patterns given in ${DEBUGLIST}
#
.for i in ${DEBUGLIST}
. for j in ${CFILES:T:M$i.c}
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

# compile rules: rules are named ${TYPE}_${SUFFIX} where TYPE is NORMAL or
# NOPROF and SUFFIX is the file suffix, capitalized (e.g. C for a .c file).
NORMAL_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO}\
		${CC} ${COPTS.${<:T}} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $< -o $@ && \
		${CC} ${COPTS.${<:T}} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $< -o $@ && \
		${COMPILE_CTFCONVERT}
NOPROF_C?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO}\
		${CC} ${COPTS.${<:T}} ${CFLAGS} ${CPPFLAGS} -c $< -o $@ && \
		${CC} ${COPTS.${<:T}} ${CFLAGS} ${CPPFLAGS} -c $< -o $@ && \
		${COMPILE_CTFCONVERT}
NORMAL_S?=	@${_MKSHMSG} "compile  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO}\
		${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${CPPFLAGS} -c $< -o $@ && \
		${CC} ${AFLAGS} ${AFLAGS.${<:T}} ${CPPFLAGS} -c $< -o $@
 
# link rules: 
LINK_O?=	@${_MKSHMSG} "   link  ${.CURDIR:T}/${.TARGET}" && \
		${_MKSHECHO}\
		${LD} -r ${LINKFORMAT} -Map=${.TARGET}.map -o ${.TARGET} ${.ALLSRC} && \
		${LD} -r ${LINKFORMAT} -Map=${.TARGET}.map -o ${.TARGET} ${.ALLSRC}

.for _s in ${CFILES}
.if !commands(${_s:T:R}.o)
${_s:T:R}.o: ${_s}
	${NORMAL_C}
.endif
.endfor

.for _s in ${SFILES}
.if !commands(${_s:T:R}.o)
${_s:T:R}.o: ${_s}
	${NORMAL_S}
.endif
.endfor

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

MI_CFILES=	param.c
.if !defined(___USE_SUFFIX_RULES___)
MI_CFILES+=	devsw.c ioconf.c
.endif

# the need for a MI_SFILES variable is dubitable at best
MI_OBJS=${MI_CFILES:S/.c/.o/}

param.c: $S/conf/param.c
	${_MKTARGET_CREATE}
	rm -f param.c
	cp $S/conf/param.c .

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

.if !empty(OBJS:Mnetbsd.ko)
SYSTEM_OBJ?=	${MD_OBJS} ${MI_OBJS} ${OBJS} ${SYSLIBCOMPAT} ${LIBKERN}
.else
SYSTEM_OBJ?=	${MD_OBJS} ${MI_OBJS} ${OBJS:O} ${SYSLIBCOMPAT} ${LIBKERN}
.endif
SYSTEM_DEP+=	Makefile ${SYSTEM_OBJ} .gdbinit
.if defined(CTFMERGE)
SYSTEM_CTFMERGE= ${CTFMERGE} ${CTFMFLAGS} -o ${.TARGET} ${SYSTEM_OBJ} ${EXTRA_OBJ} vers.o
.else
SYSTEM_CTFMERGE= ${_MKSHECHO}
.endif
SYSTEM_LD_HEAD?=@rm -f $@
SYSTEM_LD?=	@do_system_ld() { \
		target=$$1; shift; \
		${_MKSHMSG} "   link  ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${LD} -Map $${target}.map --cref ${LINKFLAGS} -o $${target} '$${SYSTEM_OBJ}' '$${EXTRA_OBJ}' vers.o $$@; \
		${LD} -Map $${target}.map --cref ${LINKFLAGS} -o $${target} ${SYSTEM_OBJ} ${EXTRA_OBJ} vers.o $$@; \
		}; \
		do_system_ld

# Give MD generated ldscript dependency on ${SYSTEM_OBJ}
.if defined(KERNLDSCRIPT)
.if target(${KERNLDSCRIPT})
${KERNLDSCRIPT}: ${SYSTEM_OBJ}
.endif
.endif

.if defined(KERNLDSCRIPT)
.for k in ${KERNELS}
EXTRA_CLEAN+=	${k}.ldscript
${k}: ${k}.ldscript
${k}.ldscript: ${KERNLDSCRIPT} assym.h
	${_MKTARGET_CREATE}
	${CPP} -I. ${KERNLDSCRIPT} | grep -v '^#' | grep -v '^$$' >$@
.endfor
LINKSCRIPT=	-T ${.TARGET}.ldscript
.endif

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

SYSTEM_LD_HEAD+=${SYSTEM_LD_HEAD_EXTRA}
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

assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf | \
	    ${GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} > assym.h.tmp && \
	mv -f assym.h.tmp assym.h
${MD_SFILES:C/\.[Ss]/.o/} ${SFILES:C/\.[Ss]/.o/}: assym.h

MKREPRO?=no

.if ${MKREPRO} == "yes"
_NVFLAGS=${NVFLAGS} -r
.else
_NVFLAGS=${NVFLAGS}
.endif

.if !target(vers.o)
newvers: vers.o
vers.o: ${SYSTEM_OBJ} Makefile $S/conf/newvers.sh \
		$S/conf/osrelease.sh ${_NETBSD_VERSION_DEPENDS}
	${_MKMSG_CREATE} vers.c
	${HOST_SH} $S/conf/newvers.sh ${_NVFLAGS}
	${_MKTARGET_COMPILE}
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c
	${COMPILE_CTFCONVERT}
.endif

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
SSRCS=${MD_SFILES} ${SFILES}
CSRCS=${MD_CFILES} ${MI_CFILES} ${CFILES}
SRCS=${SSRCS} ${CSRCS}
DEPS=	${SRCS:T:u:R:S/$/.d/g}

.for _s in ${SSRCS}
.if !target(${_s:T:R}.d)
${_s:T:R}.d: ${_s} assym.h
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_AFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
.endif
.endfor

.for _s in ${CSRCS}
.if !target(${_s:T:R}.d)
${_s:T:R}.d: ${_s}
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_CFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${_s:T}} ${_s}
.endif
.endfor

assym.d: assym.h
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} | \
	    ${GENASSYM} -- ${MKDEP} -f assym.dep -- \
	    ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${GENASSYM_CPPFLAGS}
	${TOOL_SED} -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >${.TARGET}
	rm -f assym.dep

DEPS+=	assym.d

.depend: ${DEPS}
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

.include "${S}/conf/splash.mk"
.include "${S}/conf/mdroot.mk"
.include "${S}/conf/lint.mk"
.include "${S}/conf/cscope.mk"
.include "${S}/conf/gdbinit.mk"

##
## the kernel
##

# The following files use alloca(3) or variable array allocations.
# Their full name is noted as documentation.
VARSTACK=kern/uipc_socket.c miscfs/genfs/genfs_vnops.c \
    nfs/nfs_bio.c uvm/uvm_bio.c \
    uvm/uvm_pager.c dev/ic/aic7xxx.c dev/ic/aic79xx.c arch/xen/i386/gdt.c \
    dev/ofw/ofw_subr.c

.for __varstack in ${VARSTACK}
COPTS.${__varstack:T} += -Wno-stack-protector
.endfor

AFLAGS+=	${AOPTS.${.IMPSRC:T}}
CFLAGS+=	${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}}
CPPFLAGS+=	${CPPFLAGS.${.IMPSRC:T}}
CWARNFLAGS+=	${CWARNFLAGS.${.IMPSRC:T}}

.if !defined(COPY_SYMTAB)
build_kernel: .USE
	${SYSTEM_LD_HEAD}
.if !defined(___USE_SUFFIX_RULES___)
	${SYSTEM_LD} ${.TARGET} swap${.TARGET}.o
.else
	${SYSTEM_LD} ${.TARGET}
.endif
	${SYSTEM_LD_TAIL_STAGE2}
.else
.for k in ${KERNELS}
${k}: $S/kern/kern_ksyms_buf.c
.endfor
build_kernel: .USE
	${CC} ${CFLAGS} ${CPPFLAGS} -DCOPY_SYMTAB \
	    -c $S/kern/kern_ksyms_buf.c -o kern_ksyms_buf.o
	${SYSTEM_LD_HEAD}
.if !defined(___USE_SUFFIX_RULES___)
	${SYSTEM_LD} ${.TARGET} swap${.TARGET}.o kern_ksyms_buf.o
.else
	${SYSTEM_LD} ${.TARGET} kern_ksyms_buf.o
.endif
	${SYSTEM_LD_TAIL_STAGE1}
	${CC} ${CFLAGS} ${CPPFLAGS} -DCOPY_SYMTAB \
	    -DSYMTAB_SPACE=$$(${DBSYM} -P ${.TARGET}${TARGETSFX}) \
	    -c $S/kern/kern_ksyms_buf.c -o kern_ksyms_buf_real.o
	${SYSTEM_LD_HEAD}
.if !defined(___USE_SUFFIX_RULES___)
	${SYSTEM_LD} ${.TARGET} swap${.TARGET}.o kern_ksyms_buf_real.o
.else
	${SYSTEM_LD} ${.TARGET} kern_ksyms_buf_real.o
.endif
	${SYSTEM_LD_TAIL_STAGE2}
.endif

.include <bsd.files.mk>
.include <bsd.clang-analyze.mk>

##
## suffix rules
##

.if defined(___USE_SUFFIX_RULES___)
.SUFFIXES: .genassym .assym.h
.genassym.assym.h:
	${_MKTARGET_CREATE}
	cat $< | \
	    ${GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} > $@.tmp && \
	mv -f $@.tmp $@

.SUFFIXES: .s .d
.s.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_AFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<

.SUFFIXES: .S .d
.S.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_AFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<

.SUFFIXES: .c .d
.c.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET} -- ${MKDEP_CFLAGS} \
	    ${CPPFLAGS} ${CPPFLAGS.${<:T}} $<

.SUFFIXES: .c .o
.c.o:
	${NORMAL_C}

.SUFFIXES: .s .o
.s.o:
	${NORMAL_S}

.SUFFIXES: .S .o
.S.o:
	${NORMAL_S}
.endif # ___USE_SUFFIX_RULES___

##
## the end
##
