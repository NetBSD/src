#	$NetBSD: bsd.kmodule.mk,v 1.77 2022/03/29 22:48:04 christos Exp $

# We are not building this with PIE
MKPIE=no

.include <bsd.init.mk>
.include <bsd.klinks.mk>

.if ${MKCTF:Uno} == "yes"
CFLAGS+=	-g
# Only need symbols for ctf, strip them after converting to CTF
CTFFLAGS=	-L VERSION
CTFMFLAGS=	-t -L VERSION
# Keep symbols if built with "-g"
.if !empty(COPTS:M*-g*) || ${MKDEBUG:Uno} == "yes"
CTFFLAGS+=     -g
CTFMFLAGS+=    -g
.endif
.endif

.include <bsd.sys.mk>

##### Basic targets
realinstall:	kmodinstall

KERN=		$S/kern
MKLDSCRIPT?=	no

CFLAGS+=	-ffreestanding ${COPTS}
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -isystem $S -isystem $S/arch
CPPFLAGS+=	-isystem ${S}/../common/include
CPPFLAGS+=	-D_KERNEL -D_MODULE -DSYSCTL_INCLUDE_DESCR

CWARNFLAGS.clang+=	-Wno-error=constant-conversion

# XXX until the kernel is fixed again...
CFLAGS+=	-fno-strict-aliasing -Wno-pointer-sign

# XXX This is a workaround for platforms that have relative relocations
# that, when relocated by the module loader, result in addresses that
# overflow the size of the relocation (e.g. R_PPC_REL24 in powerpc).
# The real solution to this involves generating trampolines for those
# relocations inside the loader and removing this workaround, as the
# resulting code would be much faster.
.if ${MACHINE_CPU} == "aarch64"
CFLAGS+=	-march=armv8-a+nofp+nosimd
.elif ${MACHINE_CPU} == "arm"
CFLAGS+=	-fno-common -fno-unwind-tables
.elif ${MACHINE_CPU} == "hppa"
CFLAGS+=	-mlong-calls -mno-space-regs -mfast-indirect-calls
.elif ${MACHINE_CPU} == "powerpc"
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -mlongcall :}
CFLAGS+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -mno-pltseq :}
.elif ${MACHINE_CPU} == "vax"
CFLAGS+=	-fno-pic
.elif ${MACHINE_CPU} == "riscv"
CFLAGS+=	-fPIC -Wa,-fno-pic
.elif ${MACHINE_ARCH} == "mips64eb" && !defined(BSD_MK_COMPAT_FILE)
CFLAGS+=	-mabi=64
AFLAGS+=	-mabi=64
LDFLAGS+=	-Wl,-m,elf64btsmip
.elif ${MACHINE_ARCH} == "mips64el" && !defined(BSD_MK_COMPAT_FILE)
CFLAGS+=	-mabi=64
AFLAGS+=	-mabi=64
LDFLAGS+=	-Wl,-m,elf64ltsmip
.endif

.if ${MACHINE_CPU} == "mips"
# We can't use -msym32 with -mlong-calls as -msym32 forces all addresses
# to be 32-bit which defeats the whole purpose of long calls.
CFLAGS+=	-mlong-calls
.endif

.if ${MACHINE_CPU} == "sparc64"
# force same memory model as rest of the kernel
CFLAGS+=	${${ACTIVE_CC} == "gcc":? -mcmodel=medlow :}
CFLAGS+=	${${ACTIVE_CC} == "clang":? -mcmodel=small :}
.endif

# evbppc needs some special help
.if ${MACHINE} == "evbppc"

. ifndef PPC_INTR_IMPL
PPC_INTR_IMPL=\"powerpc/intr.h\"
. endif
. ifndef PPC_PCI_MACHDEP_IMPL
PPC_PCI_MACHDEP_IMPL=\"powerpc/pci_machdep.h\"
. endif
CPPFLAGS+=	-DPPC_INTR_IMPL=${PPC_INTR_IMPL}
CPPFLAGS+=	-DPPC_PCI_MACHDEP_IMPL=${DPPC_PCI_MACHDEP_IMPL}

. ifdef PPC_IBM4XX
CPPFLAGS+=	-DPPC_IBM4XX
. elifdef PPC_BOOKE
CPPFLAGS+=	-DPPC_BOOKE
. elif ${MACHINE_ARCH} == "powerpc64"
CPPFLAGS+=	-DPPC_OEA64
. else
CPPFLAGS+=	-DPPC_OEA
. endif

.endif


_YKMSRCS=	${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}
DPSRCS+=	${_YKMSRCS}
CLEANFILES+=	${_YKMSRCS}

.if exists($S/../sys/modules/xldscripts/kmodule)
KMODSCRIPTSRC=	$S/../sys/modules/xldscripts/kmodule
.else
KMODSCRIPTSRC=	${DESTDIR}/usr/libdata/ldscripts/kmodule
.endif
.if ${MKLDSCRIPT} == "yes"
KMODSCRIPT=	kldscript
MKLDSCRIPTSH=	
.else
KMODSCRIPT=	${KMODSCRIPTSRC}
.endif

PROG?=		${KMOD}.kmod
.if ${MKDEBUG:Uno} != "no" && !defined(NODEBUG) && !commands(${PROG}) && \
    empty(SRCS:M*.sh)
PROGDEBUG:=      ${PROG}.debug
.endif  

##### Build rules
realall:	${PROG} ${PROGDEBUG}

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}

${OBJS} ${LOBJS}: ${DPSRCS}

.if ${MKLDSCRIPT} == "yes"
${KMODSCRIPT}: ${KMODSCRIPTSRC} ${OBJS} $S/conf/mkldscript.sh
	@rm -f ${.TARGET}
	@OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
	    -t ${KMODSCRIPTSRC} ${OBJS} > ${.TARGET}
.endif

.if ${MACHINE_CPU} == "arm"
# The solution to limited branch space involves generating trampolines for
# those relocations while creating the module, as the resulting code will
# be much faster and simplifies the loader.
ARCHDIR=	$S/modules/arch/${MACHINE_CPU}
ASM_H=		$S/arch/${MACHINE_CPU}/include/asm.h
CLEANFILES+=	tmp.o tmp.S ${KMOD}_tmp.o ${KMOD}_tramp.o ${KMOD}_tramp.S
${KMOD}_tmp.o: ${OBJS} ${DPADD}
	${_MKTARGET_LINK}
	${LD} -r -o tmp.o ${OBJS}
	${LD} -r \
		$$(${OBJDUMP} --syms --reloc tmp.o | \
			${TOOL_AWK} -f ${ARCHDIR}/kmodwrap.awk) \
		 -o ${.TARGET} tmp.o

${KMOD}_tramp.S: ${KMOD}_tmp.o ${ARCHDIR}/kmodtramp.awk ${ASM_H}
	${_MKTARGET_CREATE}
	${OBJDUMP} --syms --reloc ${KMOD}_tmp.o | \
		 ${TOOL_AWK} -f ${ARCHDIR}/kmodtramp.awk \
		 > tmp.S && \
	${MV} tmp.S ${.TARGET}

${PROG}: ${KMOD}_tmp.o ${KMOD}_tramp.o
	${_MKTARGET_LINK}
.if exists(${ARCHDIR}/kmodhide.awk)
	${LD} -r -Map=${.TARGET}.map \
	    -o tmp.o ${KMOD}_tmp.o ${KMOD}_tramp.o
	${OBJCOPY} \
		$$(${NM} tmp.o | ${TOOL_AWK} -f ${ARCHDIR}/kmodhide.awk) \
		tmp.o ${.TARGET} && \
	rm tmp.o
.else
	${LD} -r -Map=${.TARGET}.map \
	    -o ${.TARGET} ${KMOD}_tmp.o ${KMOD}_tramp.o
.endif
.else
${PROG}: ${OBJS} ${DPADD} ${KMODSCRIPT}
	${_MKTARGET_LINK}
	${CC} ${LDFLAGS} -nostdlib -r -Wl,-T,${KMODSCRIPT},-d \
		-Wl,-Map=${.TARGET}.map \
		-o ${.TARGET} ${OBJS}
.endif
.if defined(CTFMERGE)
	${CTFMERGE} ${CTFMFLAGS} -o ${.TARGET} ${OBJS}
.endif

.if defined(PROGDEBUG)
${PROGDEBUG}: ${PROG}
	${_MKTARGET_CREATE}
	(  ${OBJCOPY} --only-keep-debug ${PROG} ${PROGDEBUG} \
	&& ${OBJCOPY} --strip-debug -p -R .gnu_debuglink \
		--add-gnu-debuglink=${PROGDEBUG} ${PROG} \
	) || (rm -f ${PROGDEBUG}; false)
.endif

##### Install rules
.if !target(kmodinstall)
.if !defined(KMODULEDIR)
.if ${KERNEL_DIR:Uno} == "yes"
_INST_DIRS=	/netbsd
_INST_DIRS+=	/netbsd/modules
KMODULEDIR=	/netbsd/modules/${KMOD}
.else
# Ensure these are recorded properly in METALOG on unprived installes:
_OSRELEASE!=	${HOST_SH} $S/conf/osrelease.sh -k
KMODULEARCHDIR?= ${MACHINE}
_INST_DIRS=	/stand/${KMODULEARCHDIR}
_INST_DIRS+=	/stand/${KMODULEARCHDIR}/${_OSRELEASE}
_INST_DIRS+=	/stand/${KMODULEARCHDIR}/${_OSRELEASE}/modules
KMODULEDIR=	/stand/${KMODULEARCHDIR}/${_OSRELEASE}/modules/${KMOD}
.endif
.endif

_INST_DIRS+=	${KMODULEDIR}
_PROG:=		${DESTDIR}${KMODULEDIR}/${PROG} # installed path

.if defined(PROGDEBUG)
.for i in ${_INST_DIRS}
_DEBUG_INST_DIRS += ${DEBUGDIR}${i}
.endfor
_INST_DIRS += ${_DEBUG_INST_DIRS}
_PROGDEBUG:=	${DESTDIR}${DEBUGDIR}${KMODULEDIR}/${PROG}.debug
.endif

.for _P P in ${_PROG} ${PROG} ${_PROGDEBUG} ${PROGDEBUG}
.if ${MKUPDATE} == "no"
${_P}! ${P}					# install rule
.if !defined(BUILD) && !make(all) && !make(${P})
${_P}!	.MADE					# no build at install
.endif
.else
${_P}: ${P}					# install rule
.if !defined(BUILD) && !make(all) && !make(${P})
${_P}:	.MADE					# no build at install
.endif
.endif
	${_MKTARGET_INSTALL}
	dirs=${_INST_DIRS:Q}; \
	for d in $$dirs; do \
		${INSTALL_DIR} ${DESTDIR}$$d; \
	done
	${INSTALL_FILE} -o ${KMODULEOWN} -g ${KMODULEGRP} -m ${KMODULEMODE} \
		${.ALLSRC} ${.TARGET}

kmodinstall::	${_P}
.PHONY:		kmodinstall
.PRECIOUS:	${_P}				# keep if install fails
.endfor

.undef _PPROG
.undef _PPROGDEBUG
.endif # !target(kmodinstall)

##### Clean rules
CLEANFILES+= a.out [Ee]rrs mklog core *.core ${PROG} ${OBJS} ${LOBJS}
CLEANFILES+= ${PROGDEBUG}
CLEANFILES+= ${PROG}.map
.if ${MKLDSCRIPT} == "yes"
CLEANFILES+= kldscript
.endif

##### Custom rules
lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS} ${LDADD}
.endif

##### Pull in related .mk logic
LINKSOWN?= ${KMODULEOWN}
LINKSGRP?= ${KMODULEGRP}
LINKSMODE?= ${KMODULEMODE}
.include <bsd.man.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.clean.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
