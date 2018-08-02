#	$NetBSD: Makefile,v 1.9 2018/08/02 00:03:59 mrg Exp $

LIBISPRIVATE=	yes

LIB=		backend

SRCS=		${G_OBJS} ${G_out_file:T} regsub.c

# XXX: something misses these.
CLEANDIRFILES+=	${G_OBJS:.o=.d}

# Make sure we use the pre-generated C files
.l.c:
	@true

BOTH_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}
CPPFLAGS+=	${BOTH_CPPFLAGS} -DTARGET_NAME=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I${HOST_LIBIBERTYOBJ}/libiberty
HOST_CPPFLAGS+=	${BOTH_CPPFLAGS} -DGENERATOR_FILE
DPSRCS+=	insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h
CPPFLAGS+=	-I${DIST}/libgcc
CPPFLAGS+=	-I${.CURDIR}/../../lib/libgcc/libgcov/arch/${MACHINE_ARCH}

CPPFLAGS.cppdefault.c+=	-DPREFIX=\"/usr\" \
			-DNATIVE_SYSTEM_HEADER_DIR=\"${G_NATIVE_SYSTEM_HEADER_DIR}\"

HOST_CXXFLAGS+=	-std=gnu++98

HOSTPROG_CXX=	1

.include <bsd.init.mk>

HOST_LIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}

.include <bsd.lib.mk>

# Force using C++ for this
ORIGHOST_CC:=	${HOST_CC}
HOST_CC:=	${HOST_CXX}
CC:=		${CXX}
CFLAGS:=	${CXXFLAGS}

# Check PREPROCESSOR_DEFINES for cppbuiltin.c and cppdefault.c
CPPFLAGS.cppbuiltin.c+=	-DBASEVER="\"${BASEVER}"\"

#
# Independent generation programs.
#
CLEANFILES+=	gengenrtl genrtl.h \
		gengtype gtyp-gen.h gtype-desc.c gtype-desc.h \
		gtype-c.h gtype-cp.h gtype-f.h gtype-objc.h \
		genflags genflags.h \
		gencheck tree-check.h \
		insn-preds.c genpreds tm-preds.h tm_p.h cs-tm_p.h \
		auto-host.h \
		gt-*.h *.o *.lo \
		cs-config.h \
		gcov-iov gcov-iov.h \
		gtype-go.h gtype.state \
		pass-instances.def

# XXX
COPTS.varasm.c=			-Wno-error

# XXX Override the normal rule to use HOST_COMPILE.cc
.c.lo:
	${_MKTARGET_COMPILE}
	${HOST_COMPILE.cc} -o ${.TARGET}.o ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC}
	mv ${.TARGET}.o ${.TARGET}

# Headers that host objects depend on (except gen*rtl*)
HH_NORTL=	${G_tm_file_list} ${G_build_xm_include_list}
HH=		${HH_NORTL} genrtl.h insn-modes.h

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

insn-preds.c: genpreds ${G_md_file}
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

gtype-desc.c: gtype-desc.h
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
gengtype.lo gengtype-lex.lo gengtype-parse.lo gengtype-state.lo version.lo: ${HH} gtyp-gen.h config.h options.h
gengtype: gengtype.lo gengtype-lex.lo gengtype-parse.lo gengtype-state.lo build-errors.lo version.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

# XXX fix this by properly marking all the files that should
# XXX be built with -DGENERATOR_FILE
COPTS.gengtype-lex.c=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype-parse.c=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype-state.c=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE
COPTS.gengtype.c=	-UGENERATOR_FILE -DHOST_GENERATOR_FILE

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

insn-modes.c: genmodes
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
min-insn-modes.c: genmodes
	${_MKTARGET_CREATE}
	./genmodes -m >${.TARGET}
genmodes.lo: ${HH_NORTL}
genmodes: genmodes.lo build-errors.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

CLEANFILES+=	genmodes insn-modes.c insn-modes.h-test min-insn-modes.c

gimple-match.c: genmatch cfn-operators.pd
	./genmatch --gimple ${GNUHOSTDIST}/gcc/match.pd > gimple-match.c.tmp
	mv gimple-match.c.tmp gimple-match.c
generic-match.c: genmatch cfn-operators.pd
	./genmatch --generic ${GNUHOSTDIST}/gcc/match.pd > generic-match.c.tmp
	mv generic-match.c.tmp generic-match.c
genmatch.lo: ${HH_NORTL} ${G_GGC_H}
genmatch: genmatch.lo build-errors.lo build-vec.lo build-hash-table.lo
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBCPP} ${HOSTLIBIBERTY}

CLEANFILES+=	genmatch gimple-match.c generic-match.c

gcov-iov.h: gcov-iov
	${_MKTARGET_CREATE}
	./gcov-iov "${BASEVER}" "${DEVPHASE}" >${.TARGET}
gcov-iov.lo: ${HH_NORTL}
gcov-iov: gcov-iov.lo
	${_MKTARGET_CREATE}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

tm_p.h: Makefile tm-preds.h
	${_MKTARGET_CREATE}
	TARGET_CPU_DEFAULT="" \
	HEADERS="$(G_tm_p_include_list)" DEFINES="" \
	${HOST_SH} ${GNUHOSTDIST}/gcc/mkconfig.sh ${.TARGET}

# XXX crude
${G_OBJS} ${G_OBJS:.o=.d}:	tm.h tm_p.h insn-opinit.h

.include "../Makefile.options"

# Yuck, we have to run configure to generate this one...
# We use ORIGHOST_CC here because GCC's ./configure expects (and
# depends upon) it.
CLEANFILES+=	auto-build.h
HOST_CFLAGS+= -I${.OBJDIR}
HOST_CXXFLAGS+= -I${.OBJDIR}

# Set this to "true" to leave .ab around
HOST_AB_CLEAN?=	rm -rf .ab

auto-build.h: gmp.h Makefile
	${_MKTARGET_CREATE}
	rm -rf .ab && \
	mkdir .ab && \
	(cd .ab && \
		AWK=${TOOL_AWK:Q} \
		CC=${ORIGHOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} \
		CXX=${HOST_CXX:Q} CXXFLAGS=${HOST_CXXFLAGS:Q} \
		MAKE=${TOOL_GMAKE:Q} \
		CONFIG_SHELL=${HOST_SH:Q} \
		gcc_cv_ld=${LD:Q} \
		gcc_cv_as=${AS:Q} \
		${HOST_SH} ${GNUHOSTDIST}/gcc/configure \
			--build=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--host=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--target=${MACHINE_GNU_PLATFORM} && \
		mv auto-host.h ../auto-build.h) && \
	${HOST_AB_CLEAN}

pass-instances.def: passes.def gen-pass-instances.awk
	${_MKTARGET_CREATE}
	${TOOL_AWK} -f ${GNUHOSTDIST}/gcc/gen-pass-instances.awk \
	  ${GNUHOSTDIST}/gcc/passes.def > pass-instances.def
CLEANFILES+=	pass-instances.def

#
# These are copies of files we build for the build host that are used
# by the genfoo programs.
#

build-rtl.c: rtl.c \
  ${G_RTL_H} real.h ${G_GCC_H} ${G_RTL_BASE_H} ${G_GGC_H} errors.h
	${_MKTARGET_CREATE}
	rm -f build-rtl.c
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/rtl.c >${.TARGET}
build-rtl.lo: ${HH} insn-constants.h
CLEANFILES+=	build-rtl.c

build-print-rtl.c: print-rtl.c \
  ${G_RTL_BASE_H} ${G_GTM_H} hard-reg-set.h
	${_MKTARGET_CREATE}
	rm -f build-print-rtl.c
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/print-rtl.c >${.TARGET}
build-print-rtl.lo: ${HH}
build-print-rtl.o: print-rtl.c $(BCONFIG_H) $(SYSTEM_H) coretypes.h     \
  $(GTM_H) $(RTL_BASE_H)
CLEANFILES+=	build-print-rtl.c

build-bitmap.c: bitmap.c \
  ${G_RTL_H} flags.h ${G_BASIC_BLOCK_H} ${REGS_H} ${G_GCC_H} ${G_GGC_H}
	${_MKTARGET_CREATE}
	rm -f build-bitmap.c
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/bitmap.c >${.TARGET}
build-bitmap.lo: ${HH}
CLEANFILES+=	build-bitmap.c

build-errors.c: errors.c errors.h
	${_MKTARGET_CREATE}
	rm -f build-errors.c
	cat ${GNUHOSTDIST}/gcc/errors.c >${.TARGET}
build-errors.lo: ${HH_NORTL}
CLEANFILES+=	build-errors.c

build-varray.c: varray.c \
  ${G_RTL_H} ${G_GCC_H} ${TREE_H} bitmap.h errors.h
	${_MKTARGET_CREATE}
	rm -f build-varray.c
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/varray.c >${.TARGET}
build-varray.lo: ${HH} gtype-desc.h
CLEANFILES+=	build-varray.c

build-ggc-none.c: ggc-none.c \
  ${G_GCC_H} gtype-desc.h
	${_MKTARGET_CREATE}
	rm -f build-ggc-none.c
	${TOOL_SED} -e 's/<config[.]h/hconfig.h/' \
		${GNUHOSTDIST}/gcc/ggc-none.c >${.TARGET}
build-ggc-none.lo: ${HH}
CLEANFILES+=	build-ggc-none.c

build-hash-table.c: hash-table.c hash-table.h ${G_GGC_H}
	${_MKTARGET_CREATE}
	rm -f build-hash-table.c
	cat ${GNUHOSTDIST}/gcc/hash-table.c >${.TARGET}
build-hash-table.lo: ${HH_NORTL}
CLEANFILES+=	build-hash-table.c

build-vec.c: vec.c vec.h ${G_GGC_H}
	${_MKTARGET_CREATE}
	rm -f build-vec.c
	cat ${GNUHOSTDIST}/gcc/vec.c >${.TARGET}
build-vec.lo: ${HH_NORTL}
CLEANFILES+=	build-vec.c

build-genattrtab.o: ${G_GGC_H}

#
# The normal insn-foo generators
#
.for f in gensupport.c read-rtl.c dummy-conditions.c
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
gen${f}.lo: ${HH} gen${f}.c ${G_RTL_BASE_H}
gen${f}: gen${f}.lo ${GENPROG_RTL_DEPENDS} 
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}}
CLEANFILES+=	gen${f} gen${f}.lo
.endfor

.for f in mddeps constants enums
gen${f}.lo: ${HH} gen${f}.c ${G_RTL_BASE_H}
gen${f}: gen${f}.lo ${GENPROG_READER_DEPENDS}
	${_MKTARGET_LINK}
	${HOST_LINK.cc} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} ${LDFLAGS.${.TARGET}}
CLEANFILES+=	gen${f} gen${f}.lo
.endfor

.for f in check checksum
gen${f}.lo: ${HH} gen${f}.c ${G_RTL_BASE_H}
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
	  automata.c emit.c extract.c output.c peep.c recog.c \
	  target-def.h
insn-${f}: gen${f:R} ${G_md_file} insn-conditions.md
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} insn-conditions.md >${.TARGET}
CLEANFILES+=	insn-${f}
.endfor

# normal files
.for f in constants.h enums.c
insn-${f}: gen${f:R} ${G_md_file}
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} >${.TARGET}
CLEANFILES+=	insn-${f}
.endfor

# genconditions outputs gencondmd.c
gencondmd.c: genconditions ${G_md_file}
	${_MKTARGET_CREATE}
	./genconditions ${G_md_file} >${.TARGET}
CLEANFILES+=	gencondmd.c

# gencheck outputs tree-check.h
tree-check.h: gencheck
	${_MKTARGET_CREATE}
	./gencheck >${.TARGET}
CLEANFILES+=	tree-check.h

# genattrtab has three output files
insn-dfatab.c insn-latencytab.c: insn-attrtab.c
	cp -p ${.TARGET}.tmp ${.TARGET}
insn-attrtab.c: genattrtab ${G_md_file} insn-conditions.md 
	${_MKTARGET_CREATE}
	./genattrtab ${G_md_file} insn-conditions.md \
		-A${.TARGET}.tmp -Dinsn-dfatab.c.tmp -Linsn-latencytab.c.tmp && \
	mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	insn-attrtab.c insn-dfatab.c insn-latencytab.c

# genopinit has two output files
insn-opinit.h: insn-opinit.c
	cp -p ${.TARGET}.tmp ${.TARGET}
insn-opinit.c: genopinit ${G_md_file} insn-conditions.md
	${_MKTARGET_CREATE}
	./genopinit ${G_md_file} insn-conditions.md \
		-hinsn-opinit.h.tmp -c${.TARGET}.tmp && \
	mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	insn-opinit.h insn-opinit.c

LDFLAGS.genautomata+=	-lm

# gencondmd outputs a special .md file
insn-conditions.md: gencondmd
	./gencondmd >${.TARGET}
CLEANFILES+=	insn-conditions.md 

# build params.list for params.h.
params.h: params.list
${SRCS}: params.list
params-list.h: params.def
params.list: params-list.h
	${CPP} ${.ALLSRC} | sed 's/^#.*//;/^$$/d' > params.list.tmp && \
	mv params.list.tmp params.list
CLEANFILES+=	params.list.tmp params.list

# build case-cfn-macros.h and cfn-operators.pd
case-cfn-macros.h: gencfn-macros
	./gencfn-macros -c > case-cfn-macros.h.tmp && \
	mv case-cfn-macros.h.tmp case-cfn-macros.h
cfn-operators.pd: gencfn-macros
	./gencfn-macros -o > cfn-operators.pd.tmp && \
	mv cfn-operators.pd.tmp cfn-operators.pd
CLEANFILES+=	case-cfn-macros.h.tmp case-cfn-macros.h \
		cfn-operators.pd.tmp cfn-operators.pd

CASECFNDEPS=	aarch64-builtins.c arm-builtins.c i386.c rs6000.c builtins.c \
		fold-const.c tree-ssa-math-opts.c tree-ssa-reassoc.c \
		tree-vect-patterns.c tree-vrp.c generic-match-head.c \
		gimple-match-head.c ChangeLog-2015 fold-const-call.c \
		gimple-ssa-backprop.c
${CASECFNDEPS}: case-cfn-macros.h

aarch64-builtin-iterators.h: ${DIST}/gcc/config/aarch64/geniterators.sh \
    ${DIST}/gcc/config/aarch64/iterators.md
	$(HOST_SH) ${.ALLSRC} > ${.TARGET}

CLEANFILES+=	aarch-builtin-iterators.h

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
coverage.d: gcov-iov.h
gcov-io.h: gcov-iov.h
cgraph.d cgraphunit.d cgraphunit.o: gcov-io.h
vec.lo: gtype-desc.h
insn-emit.d insn-emit.o: tm-constrs.h
insn-attr.h: insn-attr-common.h
gcov-io.h: gcov-iov.h
df-scan.d df-scan.o: target-hooks-def.h
read-md.d read-md.o read-md.lo: auto-build.h
hash-table.d hash-table.o hash-table.lo: auto-build.h gtype-desc.h
gencfn-macros.d gencfn-macros.o gencfn-macros.lo: gtype-desc.h
pass_manager.h passes.c: pass-instances.def
context.d coverage.d lto-cgraph.d passes.d statistics.d toplev.d cgraphunit.d: pass_manager.h
context.o coverage.o lto-cgraph.o passes.o statistics.o toplev.o cgraphunit.o: pass_manager.h
insn-opinit.o insn-opinit.d: insn-flags.h
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
	   gcov-iov gcov libgcov lto-cgraph mcf modulo-sched profile value-prof
${_f}.o ${_f}.d: gcov-io.h
.endfor

${G_out_file:T:.c=.o}: insn-target-def.h target-hooks-def.h tm-constrs.h

# XXX make all hooks generated for now.
final.o: target-hooks-def.h c-family/c-target-hooks-def.h common/common-target-hooks-def.h

.if ${MACHINE_CPU} == "arm"
# XXX insn-recog.c:10304:7: error: this decimal constant is unsigned only in ISO C90 [-Werror]
COPTS.insn-recog.c+=-Wno-error
.endif

.if ${GCC_MACHINE_ARCH} == "i386" || ${GCC_MACHINE_ARCH} == "x86_64"
i386.d i386.o: pass_manager.h
.endif

.if ${MACHINE_CPU} == "mips"
# XXX mips.md:3474:11: error: this decimal constant is unsigned only in ISO C90 [-Werror]
COPTS.insn-recog.c+=-Wno-error
.endif

.if ${GCC_MACHINE_ARCH} == "vax"
CPPFLAGS+=-I${.CURDIR}/../../lib/libgcc/libgcov/arch/${GCC_MACHINE_ARCH}
COPTS.builtins.c+=-O0
COPTS.convert.c+=-O0
COPTS.dwarf2out.c+=-O0
COPTS.dse.c+=-O0					# XXX port-vax/51967
COPTS.expmed.c+=-O0
COPTS.expr.c+=-O0 -Wno-error=tautological-compare
COPTS.fixed-value.c+=-O0
COPTS.fold-const.c+=-O0
COPTS.gimple-fold.c+=-O0
COPTS.gimple-ssa-strength-reduction.c+=-O0
COPTS.gimple.c+=-O0
COPTS.internal-fn.c+=-O0
COPTS.lto-streamer-out.c+=-O0
COPTS.omp-low.c+=-O0
COPTS.predict.c+=-O0
COPTS.recog.c+=-O0
COPTS.sanopt.c+=-O0
COPTS.stmt.c+=-O0
COPTS.stor-layout.c+=-O0
COPTS.targhooks.c+=-O0
COPTS.tree-affine.c+=-O0
COPTS.tree-cfg.c+=-O0
COPTS.tree-eh.c+=-O0
COPTS.tree-object-size.c+=-O0
COPTS.tree-parloops.c+=-O0
COPTS.tree-predcom.c+=-O0
COPTS.tree-pretty-print.c+=-O0
COPTS.tree-ssa-alias.c+=-O0
COPTS.tree-ssa-ccp.c+=-O0
COPTS.tree-ssa-forwprop.c+=-O0
COPTS.tree-ssa-loop-ivopts.c+=-O0
COPTS.tree-ssa-loop-manip.c+=-O0
COPTS.tree-ssa-loop-niter.c+=-O0
COPTS.tree-ssa-math-opts.c+=-O0
COPTS.tree-ssa-pre.c+=-O0
COPTS.tree-ssa-reassoc.c+=-O0
COPTS.tree-ssa-uninit.c+=-O0
COPTS.tree-switch-conversion.c+=-O0
COPTS.tree-vect-data-refs.c+=-O0
COPTS.tree-vect-loop-manip.c+=-O0
COPTS.tree-vect-patterns.c+=-O0
COPTS.tree-vect-stmts.c+=-O0
COPTS.tree-vrp.c+=-O0
COPTS.tree.c+=-O0
COPTS.ubsan.c+=-O0
COPTS.varasm.c+=-O0
COPTS.wide-int.cc+=-O0
.else
COPTS.tree.c=	${${ACTIVE_CC} == "clang" :? -O0 :}
.endif

CFLAGS+=	-Wno-stack-protector

.if ${GCC_MACHINE_ARCH} == "m68000" || ${GCC_MACHINE_ARCH} == "m68k" || \
	${GCC_MACHINE_ARCH} == "coldfire"
COPTS.tree-loop-distribution.c+=-O2
COPTS.generic-match.c+=-fno-jump-tables
COPTS.gimple-match.c+=-fno-jump-tables
.endif

# Don't fight with decimal128Local.h.
# The GCC build only ignores the warning?
COPTS.dfp.c+=	-fno-strict-aliasing

CFLAGS+= -fno-exceptions -fno-rtti -fasynchronous-unwind-tables

.PATH: ${DIST}/gcc ${DIST}/libiberty ${GCCARCH} ${G_out_file:H} ${DIST}/include
.PATH: ${DIST}/gcc/config
.if ${MACHINE_ARCH} == "aarch64"
.PATH: ${DIST}/gcc/config/arm
.endif
