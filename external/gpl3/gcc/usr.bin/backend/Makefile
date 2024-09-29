#	$NetBSD: Makefile,v 1.78 2024/09/29 17:14:40 kalvisd Exp $

HOSTPROG_CXX=	1
LIBISPRIVATE=	yes

# For ../Makefile.inc and bsd.own.mk
.include <bsd.init.mk>

LIB=		backend

SRCS=		${G_OBJS:S,analyzer/,,:S,rtl-ssa/,,} ${G_out_file:T} regsub.cc

# XXX: something misses these.
CLEANDIRFILES+=	${G_OBJS:.o=.d}

# Make sure we use the pre-generated C files
.l.c:
	@true

BOTH_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}
CPPFLAGS+=	${BOTH_CPPFLAGS} -DTARGET_NAME=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I${HOST_LIBIBERTYOBJ}/libiberty
HOST_CPPFLAGS+=	${BOTH_CPPFLAGS} -DGENERATOR_FILE
DPSRCS+=	version.h
DPSRCS+=	insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h
DPSRCS+=	${DIST}/gcc/BASE-VER
CPPFLAGS+=	-I${DIST}/libgcc
CPPFLAGS+=	-I${.CURDIR}/../../lib/libgcc/libgcov/arch/${MACHINE_ARCH}

CPPFLAGS.cppdefault.cc+=	-DPREFIX=\"/usr\" \
				-DNATIVE_SYSTEM_HEADER_DIR=\"${G_NATIVE_SYSTEM_HEADER_DIR}\"
CPPFLAGS.lto-streamer-in.cc+=	-DTARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

HOST_CXXFLAGS+=	-std=gnu++11

HOST_LIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}

.include <bsd.lib.mk>

# Force using C++ for this
ORIGHOST_CC:=	${HOST_CC}
HOST_CC:=	${HOST_CXX}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

# Check PREPROCESSOR_DEFINES for cppbuiltin.cc and cppdefault.cc
CPPFLAGS.cppbuiltin.cc+=	-DBASEVER="\"${BASEVER}"\"

#
# Independent generation programs.
#
CLEANFILES+=	gengenrtl genrtl.h \
		gengtype gtyp-gen.h gtype-brig.h gtype-desc.cc gtype-desc.h \
		gtype-c.h gtype-cp.h gtype-f.h gtype-objc.h \
		gtype-d.h gtype-fortran.h \
		genflags genflags.h \
		gencheck tree-check.h \
		insn-preds.cc genpreds tm-preds.h tm_p.h cs-tm_p.h \
		auto-host.h \
		gt-*.h *.o *.lo \
		cs-config.h \
		version.h \
		gtype-go.h gtype.state \
		pass-instances.def

# XXX
#COPTS.varasm.cc=		-Wno-error

.SUFFIXES: .lo .h .c .cc

# XXX Override the normal rule to use HOST_COMPILE.cc
.c.lo:
	${_MKTARGET_COMPILE}
	${HOST_COMPILE.cc} -o ${.TARGET}.o ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
	mv ${.TARGET}.o ${.TARGET}

# Headers that host objects depend on (except gen*rtl*)
HH_NORTL=	${G_tm_file_list} ${G_build_xm_include_list}
HH=		${HH_NORTL} genrtl.h insn-modes.h insn-modes-inline.h \
		gensupport.h read-md.h optabs.def

#
# Generate the various header files we need.
#
genrtl.h: gengenrtl
	${_MKTARGET_CREATE}
	./gengenrtl >${.TARGET}
gengenrtl.lo: ${HH_NORTL}
gengenrtl: gengenrtl.lo build-errors.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

insn-preds.cc: genpreds ${G_md_file}
	${_MKTARGET_CREATE}
	./genpreds ${G_md_file} >${.TARGET}
tm-preds.h: genpreds ${G_md_file}
	${_MKTARGET_CREATE}
	./genpreds -h ${G_md_file} >${.TARGET}
tm-constrs.h: genpreds ${G_md_file}
	${_MKTARGET_CREATE}
	./genpreds -c ${G_md_file} >${.TARGET}

# be nice to generate this entirely; but difficult.
gtyp-input.list.tmp: ${GCCARCH}/gtyp-input.list
	sed s#SRCDIR#${DIST}# < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=	gtyp-input.list.tmp

# XXX
CLEANFILES+=	gtype-lto.h gtype-objcp.h tm-constrs.h gtype-jit.h

gtype-desc.cc: gtype-desc.h
gtype-desc.h: gengtype gtyp-input.list.tmp
	${_MKTARGET_CREATE}
	rm -f auto-host.h
	ln -s ${.CURDIR}/../gcc/arch/${GCC_MACHINE_ARCH}/auto-host.h .
	if [ -f ${.CURDIR}/../gcc/arch/${GCC_MACHINE_ARCH}/sysroot-suffix.h ]; then \
	  rm -f sysroot-suffix.h; \
	  ln -s ${.CURDIR}/../gcc/arch/${GCC_MACHINE_ARCH}/sysroot-suffix.h ; \
	fi
	./gengtype -S ${DIST}/gcc -I gtyp-input.list.tmp -w gtype.state
	./gengtype -r gtype.state
	# GCC 4.8 installs gtype-state and gengtype as a plugin

# gengtype is the real need for options.h
gengtype.lo gengtype-lex.lo gengtype-parse.lo gengtype-state.lo: ${HH} gtyp-gen.h config.h options.h version.h
gengtype: gengtype.lo gengtype-lex.lo gengtype-parse.lo gengtype-state.lo build-errors.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

# XXX fix this by properly marking all the files that should
# XXX be built with -DGENERATOR_FILE
COPTS.gengtype-lex.cc=		-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype-parse.cc=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype-state.cc=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype.cc=		-UGENERATOR_FILE -DHOST_GENERATOR_FILE

gtyp-gen.h: Makefile
	${_MKTARGET_CREATE}
	(\
	unset empty							; \
	echo "static const char *srcdir = \"$(G_GTFILES_SRCDIR)\";"	; \
	echo "static const char *lang_files[] = {"			; \
	for f in $$empty $(G_GTFILES_FILES_FILES); do echo "\"$$f\", "; done ; \
	echo "NULL };"							; \
	echo "static const char *langs_for_lang_files[] = {"		; \
	for f in $$empty $(G_GTFILES_FILES_LANGS); do echo "\"$$f\", "; done ; \
	echo "NULL };"							; \
	echo "static const char *all_files[] = {"			; \
	for f in $$empty $(G_GTFILES); do echo "\"$$f\", "; done	; \
	echo " NULL };"							; \
	echo "static const char *lang_dir_names[] = {"			; \
	for f in c $(G_GTFILES_LANG_DIR_NAMES); do echo "\"$$f\", "; done ; \
	echo "NULL };"							; \
	) >${.TARGET}

insn-modes.cc: genmodes
	${_MKTARGET_CREATE}
	./genmodes >${.TARGET}
insn-modes.h-test: genmodes
	${_MKTARGET_CREATE}
	./genmodes -h >${.TARGET}
	# Ensure that the checked in version matches
	if ! cmp ${.TARGET} ${.CURDIR}/../gcc/arch/${MACHINE_ARCH}/insn-modes.h; then
		echo "Checked in insn-modes.h does not match; rerun mknative-gcc." 1>&2
		false
	fi
min-insn-modes.cc: genmodes
	${_MKTARGET_CREATE}
	./genmodes -m >${.TARGET}
insn-modes-inline.h: genmodes
	${_MKTARGET_CREATE}
	./genmodes -i >${.TARGET}
genmodes.lo: ${HH_NORTL}
genmodes: genmodes.lo build-errors.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

CLEANFILES+=	genmodes insn-modes.cc insn-modes.h-test min-insn-modes.cc insn-modes-inline.h

gimple-match.cc: genmatch cfn-operators.pd
	./genmatch --gimple ${GNUHOSTDIST}/gcc/match.pd > gimple-match.cc.tmp
	mv gimple-match.cc.tmp gimple-match.cc
generic-match.cc: genmatch cfn-operators.pd
	./genmatch --generic ${GNUHOSTDIST}/gcc/match.pd > generic-match.cc.tmp
	mv generic-match.cc.tmp generic-match.cc
genmatch.lo: ${HH_NORTL} ${G_GGC_H} case-cfn-macros.h
genmatch: genmatch.lo build-errors.lo build-vec.lo build-hash-table.lo build-sort.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBCPP} ${HOSTLIBIBERTY}

CLEANFILES+=	genmatch gimple-match.cc generic-match.cc

.include "${.PARSEDIR}/../../Makefile.version_h"
.include "${.PARSEDIR}/../Makefile.isl"

tm_p.h: Makefile tm-preds.h
	${_MKTARGET_CREATE}
	TARGET_CPU_DEFAULT="" \
	HEADERS="$(G_tm_p_include_list)" DEFINES="" \
	${HOST_SH} ${GNUHOSTDIST}/gcc/mkconfig.sh ${.TARGET}

# XXX crude
${G_OBJS} ${G_OBJS:.o=.d}:	tm.h tm_p.h insn-opinit.h

.include "../Makefile.options"
.include "../../Makefile.autobuild_h"

pass-instances.def: passes.def gen-pass-instances.awk
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${GNUHOSTDIST}/gcc/gen-pass-instances.awk \
	  ${GNUHOSTDIST}/gcc/passes.def > pass-instances.def
CLEANFILES+=	pass-instances.def

#
# These are copies of files we build for the build host that are used
# by the genfoo programs.
#

build-rtl.cc: rtl.cc \
  ${G_RTL_H} real.h ${G_GCC_H} ${G_RTL_BASE_H} ${G_GGC_H} errors.h
	${_MKTARGET_CREATE}
	rm -f build-rtl.cc
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/rtl.cc >${.TARGET}
build-rtl.lo: ${HH} insn-constants.h
CLEANFILES+=	build-rtl.cc

build-print-rtl.cc: print-rtl.cc \
  ${G_RTL_BASE_H} ${G_GTM_H} hard-reg-set.h
	${_MKTARGET_CREATE}
	rm -f build-print-rtl.cc
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/print-rtl.cc >${.TARGET}
build-print-rtl.lo: ${HH}
build-print-rtl.o: print-rtl.cc $(BCONFIG_H) $(SYSTEM_H) coretypes.h     \
  ${GTM_H} ${G_RTL_BASE_H}
CLEANFILES+=	build-print-rtl.cc

build-bitmap.cc: bitmap.cc \
  ${G_RTL_H} flags.h ${G_BASIC_BLOCK_H} ${REGS_H} ${G_GCC_H} ${G_GGC_H}
	${_MKTARGET_CREATE}
	rm -f build-bitmap.cc
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/bitmap.cc >${.TARGET}
build-bitmap.lo: ${HH}
CLEANFILES+=	build-bitmap.cc

build-errors.cc: errors.cc errors.h
	${_MKTARGET_CREATE}
	rm -f build-errors.cc
	cat ${GNUHOSTDIST}/gcc/errors.cc >${.TARGET}
build-errors.lo: ${HH_NORTL}
CLEANFILES+=	build-errors.cc

build-varray.cc: varray.cc \
  ${G_RTL_H} ${G_GCC_H} ${TREE_H} bitmap.h errors.h
	${_MKTARGET_CREATE}
	rm -f build-varray.cc
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/varray.cc >${.TARGET}
build-varray.lo: ${HH} gtype-desc.h
CLEANFILES+=	build-varray.cc

build-ggc-none.cc: ggc-none.cc \
  ${G_GCC_H} gtype-desc.h
	${_MKTARGET_CREATE}
	rm -f build-ggc-none.cc
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/ggc-none.cc >${.TARGET}
build-ggc-none.lo: ${HH}
CLEANFILES+=	build-ggc-none.cc

build-hash-table.cc: hash-table.cc hash-table.h ${G_HASH_TABLE_H} ${HH}
	${_MKTARGET_CREATE}
	rm -f build-hash-table.cc
	cat ${GNUHOSTDIST}/gcc/hash-table.cc >${.TARGET}
build-hash-table.lo: ${HH_NORTL}
CLEANFILES+=	build-hash-table.cc

build-vec.cc: vec.cc vec.h ${G_HASH_TABLE_H} ${HH}
	${_MKTARGET_CREATE}
	rm -f build-vec.cc
	cat ${GNUHOSTDIST}/gcc/vec.cc >${.TARGET}
build-vec.lo: ${HH_NORTL}
CLEANFILES+=	build-vec.cc

build-sort.cc: sort.cc sort.h ${G_GGC_H}
	${_MKTARGET_CREATE}
	rm -f build-sort.cc
	cat ${GNUHOSTDIST}/gcc/sort.cc >${.TARGET}
build-sort.lo: ${HH_NORTL}
CLEANFILES+=	build-sort.cc

build-genattrtab.o: ${G_GGC_H}

#
# The normal insn-foo generators
#
.for f in gensupport.cc read-rtl.cc dummy-conditions.cc read-md.cc
${f:R}.lo: ${HH}
.endfor

#
# There are 5 main classes of generator programs.  Ones
# that are self contained, ones that use error reporting
# mechanisms, ones that use the MD reader, and ones that
# use the RTL reader.
#
GENPROG_ERROR_DEPENDS=	${G_BUILD_ERRORS:.o=.lo}
GENPROG_READER_DEPENDS=	${G_BUILD_MD:.o=.lo} ${GENPROG_ERROR_DEPENDS}
GENPROG_RTL_DEPENDS=	${G_BUILD_RTL:.o=.lo} ${GENPROG_READER_DEPENDS} ${GENPROG_ERROR_DEPENDS}

#
# First we generate the rules for the generators.
#
.for f in attr attr-common attrtab automata codes conditions config emit \
	  extract flags opinit output peep preds recog mddump condmd \
	  target-def cfn-macros
gen${f}.lo: ${HH} gen${f}.cc ${G_RTL_BASE_H} build-sort.lo
gen${f}: gen${f}.lo ${GENPROG_RTL_DEPENDS} 
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}} build-sort.lo
CLEANFILES+=	gen${f} gen${f}.lo
.endfor

.for f in mddeps constants enums
gen${f}.lo: ${HH} gen${f}.cc ${G_RTL_BASE_H}
gen${f}: gen${f}.lo ${GENPROG_READER_DEPENDS}
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}}
CLEANFILES+=	gen${f} gen${f}.lo
.endfor

.for f in check checksum
gen${f}.lo: ${HH} gen${f}.cc ${G_RTL_BASE_H}
gen${f}: gen${f}.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}}
CLEANFILES+=	gen${f} gen${f}.lo
.endfor

.include "../../Makefile.hooks"

#
# Second we generate the rules for the generated files.  There
# are 3 main classes:  RTL based or not, plus special cases.
#
# The special cases are:  gencheck, genattrtab, genopinit,
# gencondmd
#

# RTL based files
.for f in attr.h attr-common.h codes.h config.h flags.h \
	  automata.cc emit.cc extract.cc output.cc peep.cc recog.cc \
	  target-def.h
insn-${f}: gen${f:R} ${G_md_file} insn-conditions.md
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} insn-conditions.md >${.TARGET}
CLEANFILES+=	insn-${f}
.endfor

# normal files
.for f in constants.h enums.cc
insn-${f}: gen${f:R} ${G_md_file}
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} >${.TARGET}
CLEANFILES+=	insn-${f}
.endfor

# genconditions outputs gencondmd.cc
gencondmd.cc: genconditions ${G_md_file}
	${_MKTARGET_CREATE}
	./genconditions ${G_md_file} >${.TARGET}
CLEANFILES+=	gencondmd.cc

# gencheck outputs tree-check.h
tree-check.h: gencheck
	${_MKTARGET_CREATE}
	./gencheck >${.TARGET}
CLEANFILES+=	tree-check.h

# genattrtab has three output files
insn-dfatab.cc insn-latencytab.cc: insn-attrtab.cc
	cp -p ${.TARGET}.tmp ${.TARGET}
insn-attrtab.cc: genattrtab ${G_md_file} insn-conditions.md 
	${_MKTARGET_CREATE}
	./genattrtab ${G_md_file} insn-conditions.md \
		-A${.TARGET}.tmp -Dinsn-dfatab.cc.tmp -Linsn-latencytab.cc.tmp && \
	mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	insn-attrtab.cc insn-dfatab.cc insn-latencytab.cc \
		insn-dfatab.cc.tmp insn-latencytab.cc.tmp

# genopinit has two output files
insn-opinit.h: insn-opinit.cc
	cp -p ${.TARGET}.tmp ${.TARGET}
insn-opinit.cc: genopinit ${G_md_file} insn-conditions.md
	${_MKTARGET_CREATE}
	./genopinit ${G_md_file} insn-conditions.md \
		-hinsn-opinit.h.tmp -c${.TARGET}.tmp && \
	mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	insn-opinit.h insn-opinit.cc insn-opinit.h.tmp

LDFLAGS.genautomata+=	-lm

# gencondmd outputs a special .md file
insn-conditions.md: gencondmd
	./gencondmd >${.TARGET}
CLEANFILES+=	insn-conditions.md 

# build case-cfn-macros.h and cfn-operators.pd
case-cfn-macros.h: gencfn-macros
	./gencfn-macros -c > case-cfn-macros.h.tmp && \
	mv case-cfn-macros.h.tmp case-cfn-macros.h
cfn-operators.pd: gencfn-macros
	./gencfn-macros -o > cfn-operators.pd.tmp && \
	mv cfn-operators.pd.tmp cfn-operators.pd
CLEANFILES+=	case-cfn-macros.h.tmp case-cfn-macros.h \
		cfn-operators.pd.tmp cfn-operators.pd

CASECFNDEPS=	aarch64-builtins.cc arm-builtins.cc i386.cc rs6000.cc \
		riscv-builtins.cc builtins.cc \
		fold-const.cc tree-ssa-math-opts.cc tree-ssa-reassoc.cc \
		tree-vect-patterns.cc tree-vrp.cc generic-match-head.cc \
		gimple-match-head.cc fold-const-call.cc \
		gimple-ssa-backprop.cc
${CASECFNDEPS}: case-cfn-macros.h

aarch64-builtin-iterators.h: ${DIST}/gcc/config/aarch64/geniterators.sh \
    ${DIST}/gcc/config/aarch64/iterators.md
	$(HOST_SH) ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	aarch-builtin-iterators.h

.if ${GCC_MACHINE_ARCH} == "powerpc" || ${GCC_MACHINE_ARCH} == "powerpc64" # {

.include "../rs6000-inc.mk"
gtype-desc.h: rs6000-builtins.h

.endif # }


#
# Required hard-coded dependancies.
#
aarch64-builtins.o: aarch64-builtin-iterators.h
tm.h: insn-constants.h
genextract.lo: insn-config.h
gencondmd.lo: tm_p.h tm-constrs.h
read-rtl.lo gencodes.lo genattrtab.lo genautomata.lo: tm.h gtype-desc.h
genattr-common.lo gensupport.lo genconditions.lo gentarget-def.lo: tm.h
genattr.lo genflags.lo genemit.lo genconfig.lo gencheck.lo: gtype-desc.h tm.h
genopinit.lo genrecog.lo genpreds.lo genoutput.lo genpeep.lo: tm.h
build-ggc-none.lo: gtype-desc.h
ggc-none.o: gtype-desc.h
insn-attrtab.o: insn-config.h
insn-extract.o: insn-config.h
toplev.d toplev.o: options.h
coverage.d: version.h
gcov-io.h: version.h
cgraph.d cgraphunit.d cgraphunit.o: gcov-io.h
vec.lo: gtype-desc.h
insn-emit.d insn-emit.o: tm-constrs.h
insn-attr.h: insn-attr-common.h
gcov-io.h: version.h
df-scan.d df-scan.o: target-hooks-def.h
read-md.d read-md.o read-md.lo: auto-build.h
hash-table.d hash-table.o hash-table.lo: auto-build.h gtype-desc.h
gencfn-macros.d gencfn-macros.o gencfn-macros.lo: gtype-desc.h
pass_manager.h passes.cc: pass-instances.def
context.d coverage.d lto-cgraph.d passes.d statistics.d toplev.d cgraphunit.d: pass_manager.h
context.o coverage.o lto-cgraph.o passes.o statistics.o toplev.o cgraphunit.o: pass_manager.h
insn-opinit.o insn-opinit.d: insn-flags.h
gensupport.o: ${G_HASH_TABLE_H}
vec.o: ${G_HASH_TABLE_H}
.for _f in insn-attrtab insn-automata insn-dfatab insn-latencytab insn-output
${_f}.o ${_f}.d: insn-attr-common.h insn-attr.h
.endfor
.for _f in insn-attrtab insn-automata insn-dfatab insn-emit insn-extract \
	   insn-latencytab insn-opinit insn-output insn-peep insn-preds \
	   insn-recog
${_f}.o ${_f}.d: insn-codes.h insn-config.h insn-constants.h insn-flags.h
.endfor
.for _f in insn-enums insn-modes
${_f}.o ${_f}.d: insn-constants.h
.endfor
.for _f in data-streamer data-streamer-out coverage gcov-dump gcov-io \
	   gcov libgcov lto-cgraph mcf modulo-sched profile value-prof
${_f}.o ${_f}.d: gcov-io.h
.endfor

${G_out_file:T:.cc=.o}: insn-target-def.h target-hooks-def.h tm-constrs.h

# XXX make all hooks generated for now.
final.o: target-hooks-def.h c-family/c-target-hooks-def.h common/common-target-hooks-def.h

#COPTS.ira-color.cc+= -Wno-error=int-in-bool-context

.if ${MACHINE_CPU} == "arm"
# XXX insn-recog.cc:10304:7: error: this decimal constant is unsigned only in ISO C90 [-Werror]
#COPTS.insn-recog.cc+=-Wno-error
.endif

.if ${GCC_MACHINE_ARCH} == "i386" || ${GCC_MACHINE_ARCH} == "x86_64"
i386.d i386.o: pass_manager.h
.endif

.if ${MACHINE_CPU} == "mips"
# XXX mips.md:3474:11: error: this decimal constant is unsigned only in ISO C90 [-Werror]
#COPTS.insn-recog.cc+=-Wno-error
.endif

.if ${GCC_MACHINE_ARCH} == "vax"
CPPFLAGS+=-I${.CURDIR}/../../lib/libgcc/libgcov/arch/${GCC_MACHINE_ARCH}

COPTS.expmed.c+=-Wno-error=tautological-compare
COPTS.expr.c+=-Wno-error=tautological-compare
.endif

COPTS.tree.cc+=	${${ACTIVE_CC} == "clang" :? -O0 :}

CFLAGS+=	-Wno-stack-protector

#COPTS.dwarf2out.cc+=${CC_WNO_MAYBE_UNINITIALIZED}

.if ${GCC_MACHINE_ARCH} == "m68000" || ${GCC_MACHINE_ARCH} == "m68k" || \
	${GCC_MACHINE_ARCH} == "coldfire"
#COPTS.tree-loop-distribution.cc+=-O2
#COPTS.generic-match.cc+=-fno-jump-tables
#COPTS.gimple-match.cc+=-fno-jump-tables
.endif

# XXXGCC12
COPTS.diagnostic-manager.cc+=	-Wno-format-extra-args -Wno-format

# Don't fight with decimal128Local.h.
# The GCC build only ignores the warning?
COPTS.dfp.cc+=	-fno-strict-aliasing

CFLAGS+= -fno-exceptions -fno-rtti -fasynchronous-unwind-tables

.PATH: ${DIST}/gcc ${DIST}/libiberty ${GCCARCH} ${G_out_file:H} ${DIST}/include
.PATH: ${DIST}/gcc/config
.PATH: ${DIST}/gcc/analyzer
.PATH: ${DIST}/gcc/rtl-ssa
.if ${MACHINE_CPU} == "aarch64"
.PATH: ${DIST}/gcc/config/arm
.endif

.if ${MACHINE} == "vax"
CPPFLAGS+=	-DNB_FIX_VAX_BACKEND
.endif
