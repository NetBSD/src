#	$NetBSD: Makefile,v 1.164 2012/12/02 12:10:43 apb Exp $

.include <bsd.own.mk>

# Make sure that the ordered build/install processing applies when using
# plain make.
.MAIN: build_install

# TOOLDIR must be valid, unless MKTOOLS=no
.if ${MKTOOLS:Uyes} != "no"
.if "${TOOLDIR}" == ""
.error "TOOLDIR is undefined or empty"
.elif "${TOOLDIR:tW:M/*}" == ""
.error "TOOLDIR is not an absolute path: ${TOOLDIR}"
#.elif !exists(TOOLDIR) # XXX .exists fails for directories
#.error "TOOLDIR does not exist: ${TOOLDIR}"
.endif
.endif # MKTOOLS != no

.if ${TOOLCHAIN_MISSING} == "no"
.if defined(HAVE_GCC)
TOOLCHAIN_BITS= gmake .WAIT
.endif

.if defined(HAVE_GCC)
.if ${HAVE_GCC} >= 45
TOOLCHAIN_BITS+= gmp .WAIT
TOOLCHAIN_BITS+= mpfr .WAIT
TOOLCHAIN_BITS+= mpc .WAIT
.endif
.endif
.endif

.if ${TOOLCHAIN_MISSING} == "no"
TOOLCHAIN_BITS+= binutils .WAIT
.endif

.if defined(HAVE_GCC)
.if ${TOOLCHAIN_MISSING} == "no"
TOOLCHAIN_BITS+= gcc
.  if ${MKCROSSGDB:Uno} != "no"
TOOLCHAIN_BITS+= gdb
.  endif
TOOLCHAIN_BITS+= .WAIT
.endif
.endif

.if defined(HAVE_PCC)
.if ${TOOLCHAIN_MISSING} == "no"
TOOLCHAIN_BITS+= pcc
.endif
.endif

.if ${TOOLCHAIN_MISSING} == "no"
# XXX Eventually, we want to be able to build dbsym and mdsetimage
# XXX if EXTERNAL_TOOLCHAIN is set.
TOOLCHAIN_BITS+= dbsym mdsetimage
.endif

DTRACE_BITS=
.if ${MKDTRACE} != "no"
DTRACE_BITS+= .WAIT libelf
DTRACE_BITS+= .WAIT libdwarf
DTRACE_BITS+= .WAIT libctf
DTRACE_BITS+= .WAIT ctfconvert ctfmerge
.endif

LINT_BITS=
.if ${MKLINT} != "no"
LINT_BITS= lint lint2
.endif

# Dependencies in SUBDIR below ordered to maximize parallel ability.
SUBDIR=	host-mkdep .WAIT compat .WAIT \
	binstall .WAIT mktemp .WAIT sed .WAIT \
		cap_mkdb crunchgen ctags genassym gencat hexdump join \
		${LINT_BITS} \
		lorder makewhatis mkdep mtree nbperf .WAIT rpcgen tsort \
		uudecode m4 \
	.WAIT texinfo \
	.WAIT yacc \
	.WAIT awk \
	.WAIT tic \
	.WAIT lex \
	.WAIT pax \
	.WAIT ${TOOLCHAIN_BITS} \
	${DTRACE_BITS} \
		asn1_compile cat cksum compile_et config db \
		file lint1 slc \
		makefs .WAIT menuc mkcsmapper mkesdb mklocale mknod msgc \
		.WAIT disklabel \
		.WAIT paxctl \
		.WAIT fdisk \
		.WAIT installboot \
		pwd_mkdb stat strfile sunlabel zic

.if ${MKLLVM} != "no"
SUBDIR+= \
	llvm .WAIT \
	llvm-lib/libLLVMSupport llvm-lib/libLLVMTableGen .WAIT \
	llvm-tblgen llvm-clang-tblgen .WAIT \
	llvm-include .WAIT \
	llvm-lib .WAIT \
	llvm-clang
.endif

.if ${MKMAN} != "no" || ${MKDOC} != "no" || ${MKHTML} != "no"
.  if ${MKGROFF} != "no"
SUBDIR+=	groff
.  endif
SUBDIR+=	mandoc
.endif

.if ${MKMAINTAINERTOOLS:Uno} != "no"
SUBDIR+=	autoconf .WAIT gettext
.endif

.if ${USE_PIGZGZIP} != "no"
SUBDIR+=	pigz
.endif

.if ${MACHINE} == "hp700"
SUBDIR+=	hp700-mkboot
.endif

.if ${MACHINE} == "ibmnws"
SUBDIR+=	ibmnws-ncdcs
.endif

.if ${MACHINE} == "macppc"
SUBDIR+=	macppc-fixcoff
.endif

.if (${MACHINE} == "prep" || ${MACHINE} == "rs6000" || ${MACHINE} == "bebox")
SUBDIR+=	powerpc-mkbootimage
.endif

.if ${MACHINE_ARCH} == "m68k"
SUBDIR+=	m68k-elf2aout
.endif

.if (${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb" || \
     ${MACHINE_ARCH} == "mips64el" || ${MACHINE_ARCH} == "mips64eb")
SUBDIR+=	mips-elf2ecoff
.endif

.if (${MACHINE} == "sgimips")
SUBDIR+=	sgivol
.endif

.if ${MACHINE} == "acorn32"
SUBDIR+=	sparkcrc
.endif

.if (${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64")
SUBDIR+=	fgen
.endif

.if ${MACHINE} == "amiga"
SUBDIR+=	amiga-elf2bb
SUBDIR+=	amiga-txlt
.endif

.if ${MACHINE} == "hp300"
SUBDIR+=	hp300-mkboot
.endif

.if ${MACHINE} == "evbarm" && ${MACHINE_ARCH} == "arm"
SUBDIR+=	elftosb
.endif

.if ${MACHINE} == "evbarm" || ${MACHINE} == "evbmips" || \
    ${MACHINE} == "evbppc" || ${MACHINE} == "sandpoint"
SUBDIR+=	mkubootimage
.endif

check_MKTOOLS: .PHONY .NOTMAIN
.if ${MKTOOLS:Uyes} == "no"
	@echo '*** WARNING: "MKTOOLS" is set to "no"; this will prevent building and'
	@echo '*** updating your host toolchain.  This should be used only as a'
	@echo '*** temporary workaround for toolchain problems, as it will result'
	@echo '*** in version skew and build errors over time!'
.endif

.if ${MKTOOLS:Uyes} == "no" || ${USETOOLS} != "yes"	# {
SUBDIR= # empty
realall realdepend install: check_MKTOOLS
.endif							# }

.include <bsd.subdir.mk>
.include <bsd.buildinstall.mk>
.include <bsd.obj.mk>

.if !defined(PREVIOUSTOOLDIR)
.  if exists(PREVIOUSTOOLDIR)
PREVIOUSTOOLDIR!=	cat PREVIOUSTOOLDIR
.  else
PREVIOUSTOOLDIR=
.  endif
.endif

CLEANFILES+=	PREVIOUSTOOLDIR

realall realdepend: .MAKE
.if !empty(PREVIOUSTOOLDIR) && "${PREVIOUSTOOLDIR}" != "${TOOLDIR}"
	@echo "*** WARNING: TOOLDIR has moved?"
	@echo "*** PREVIOUSTOOLDIR '${PREVIOUSTOOLDIR}'"
	@echo "***     !=  TOOLDIR '${TOOLDIR}'"
	@echo "*** Cleaning mis-matched tools"
	rm -f PREVIOUSTOOLDIR
	(cd ${.CURDIR} && ${MAKE} PREVIOUSTOOLDIR=${TOOLDIR} cleandir)
.endif
	echo ${TOOLDIR} >PREVIOUSTOOLDIR

cleandir:
	rm -f ${CLEANFILES}
