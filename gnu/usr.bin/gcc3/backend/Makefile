#	$NetBSD: Makefile,v 1.14.2.1 2004/04/28 06:02:53 jmc Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		backend

SRCS=		${G_OBJS:.o=.c} ${G_out_file:T}

CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DTARGET_NAME=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}
DPSRCS+=	insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

.include <bsd.lib.mk>

# Don't auto-frob .y or .l files.
.l.c .y.c .y.h:
	@true

#
# Independent generation programs.
#
CLEANFILES+=	gengenrtl genrtl.c genrtl.h \
		gengtype gtyp-gen.h gtype-desc.c gtype-desc.h \
		gtype-c.h gtype-cp.h gtype-f.h gtype-objc.h \
		genflags genflags.h \
		gencheck tree-check.h \
		genpreds tm-preds.h tm_p.h cs-tm_p.h \
		auto-host.h print-rtl1.c \
		build-rtl.c build-errors.c build-varray.c \
		build-bitmap.c build-ggc-none.c \
		gt-*.h *.o *.lo \
		config.h cs-config.h \

#
# These files have special needs
#
CPPFLAGS.build-errors.c=	-DGENERATOR_FILE
CPPFLAGS.build-ggc-none.c=	-DGENERATOR_FILE
CPPFLAGS.build-rtl.c=		-DGENERATOR_FILE
CPPFLAGS.insn-conditions.c=	-DGENERATOR_FILE
CPPFLAGS.print-rtl1.c=		-DGENERATOR_FILE

# XXX
.if ${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "m68000"
COPTS.ra-colorize.c=		-O0
.endif

# XXX
.if ${MACHINE_CPU} == "sh5"
NOGCCERROR=			# defined
COPTS.reload1.c=		-O1
.endif



# Headers that host objects depend on (except gen*rtl*)
HH=	${G_HCONFIG_H} genrtl.h

#
# Generate the various header files we need.
#
genrtl.c: gengenrtl
	${_MKTARGET_CREATE}
	./gengenrtl >${.TARGET}
genrtl.h: gengenrtl
	${_MKTARGET_CREATE}
	./gengenrtl -h >${.TARGET}
gengenrtl.lo: ${G_HCONFIG_H}
gengenrtl: gengenrtl.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

tm-preds.h: genpreds
	${_MKTARGET_CREATE}
	./genpreds >${.TARGET}
genpreds.lo: ${HH}
genpreds: genpreds.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

tree-check.h: gencheck
	${_MKTARGET_CREATE}
	./gencheck >${.TARGET}
gencheck.lo: ${HH}
gencheck: gencheck.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

gtype-desc.c: gtype-desc.h
gtype-desc.h: gengtype
	${_MKTARGET_CREATE}
	rm -f auto-host.h
	ln -s ${.CURDIR}/../arch/${MACHINE_ARCH}/auto-host.h .
	./gengtype
gengtype.lo gengtype-lex.lo gengtype-yacc.lo: ${HH} gtyp-gen.h config.h
gengtype: gengtype.lo gengtype-lex.lo gengtype-yacc.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}

gtyp-gen.h: Makefile
	${_MKTARGET_CREATE}
	(\
	echo "static const char *srcdir = \"$(G_GTFILES_SRCDIR)\";"	; \
	echo "static const char *lang_files[] = {"			; \
	for f in $(G_GTFILES_FILES_FILES); do echo "\"$$f\", "; done	; \
	echo "NULL };"							; \
	echo "static const char *langs_for_lang_files[] = {"		; \
	for f in $(G_GTFILES_FILES_LANGS); do echo "\"$$f\", "; done 	; \
	echo "NULL };"							; \
	echo "static const char *all_files[] = {"			; \
	for f in $(G_GTFILES); do echo "\"$$f\", "; done		; \
	echo " NULL };"							; \
	echo "static const char *lang_dir_names[] = {"			; \
	echo "\"c\", "							; \
	for f in $(G_GTFILES_LANG_DIR_NAMES); do echo "\"$$f\", "; done	; \
	echo "NULL };"							; \
	) >${.TARGET}

config.h: Makefile
	${_MKTARGET_CREATE}
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_host_xm_file)" XM_DEFINES="$(G_host_xm_defines)" \
	TARGET_CPU_DEFAULT="$(G_target_cpu_default)" \
	${HOST_SH} ${GNUHOSTDIST}/gcc/mkconfig.sh ${.TARGET}

tm_p.h: Makefile
	${_MKTARGET_CREATE}
	TM_DEFINES="" \
	HEADERS="$(G_tm_p_file)" XM_DEFINES="" TARGET_CPU_DEFAULT="" \
	${HOST_SH} ${GNUHOSTDIST}/gcc/mkconfig.sh ${.TARGET}

CLEANFILES+=	options.h
options.h: Makefile
	${_MKTARGET_CREATE}
	for f in $(lang_options_files) ""; do \
		if [ -n "$$f" ] ; then echo "#include \"$$f\""; fi; \
	done | sed 's|$(GNUHOSTDIST)/gcc/||' >${.TARGET}


# Yuck, we have to run configure to generate this one...
CLEANFILES+=	auto-build.h
auto-build.h: Makefile
	${_MKTARGET_CREATE}
	rm -rf .ab && \
	mkdir .ab && \
	(cd .ab && \
		${HOST_SH} ${GNUHOSTDIST}/gcc/configure \
			--build=`${.CURDIR}/../../../dist/gcc/config.guess` \
			--host=`${.CURDIR}/../../../dist/gcc/config.guess` \
			--target=${MACHINE_GNU_PLATFORM} && \
		mv auto-host.h ../auto-build.h) && \
	rm -rf .ab

#
# These are copies of files we build for the build host that are used
# by the genfoo programs.
#

build-rtl.c: rtl.c \
  ${G_RTL_H} real.h ${G_GCC_H} errors.h gtype-desc.h
	${_MKTARGET_CREATE}
	rm -f build-rtl.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/rtl.c >${.TARGET}
build-rtl.lo: ${HH}

print-rtl1.c: print-rtl.c \
  ${G_RTL_H} ${G_TREE_H} hard-reg-set.h ${G_BASIC_BLOCK_H}
	${_MKTARGET_CREATE}
	rm -f print-rtl1.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/print-rtl.c >${.TARGET}
print-rtl1.lo: ${HH}

build-bitmap.c: bitmap.c \
  ${G_RTL_H} flags.h ${G_BASIC_BLOCK_H} ${REGS_H} ${G_GCC_H} gtype-desc.h
	${_MKTARGET_CREATE}
	rm -f build-bitmap.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/bitmap.c >${.TARGET}
build-bitmap.lo: ${HH}

build-errors.c: errors.c errors.h
	${_MKTARGET_CREATE}
	rm -f build-errors.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/errors.c >${.TARGET}
build-errors.lo: ${HH}

build-varray.c: varray.c \
  ${G_RTL_H} ${G_GCC_H} ${TREE_H} bitmap.h errors.h
	${_MKTARGET_CREATE}
	rm -f build-varray.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/varray.c >${.TARGET}
build-varray.lo: ${HH} gtype-desc.h

build-ggc-none.c: ggc-none.c \
  ${G_GCC_H} gtype-desc.h
	${_MKTARGET_CREATE}
	rm -f build-ggc-none.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/ggc-none.c >${.TARGET}
build-ggc-none.lo: ${HH}

#
# The `early' insn-foo generators
#
EXTRADEPENDSEARLY=	${G_HOST_EARLY_SUPPORT:.o=.lo} ${G_HOST_RTL:.o=.lo} \
			${G_HOST_ERRORS:.o=.lo}
.for f in conditions.c constants.h
CLEANFILES+=		gen${f:R} insn-${f}

gen${f:R}.lo: gen${f:R}.c ${HH}
gen${f:R}: gen${f:R}.lo ${EXTRADEPENDSEARLY}
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}
insn-${f}: gen${f:R} ${G_md_file}
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} >${.TARGET}
.endfor

#
# The normal insn-foo generators
#
.for f in gensupport.c read-rtl.c dummy-conditions.c
${f:R}.lo: ${HH}
.endfor

EXTRADEPENDS=		${G_HOST_RTL:.o=.lo} ${G_HOST_SUPPORT:.o=.lo} \
			${G_HOST_PRINT:.o=.lo} ${G_HOST_ERRORS:.o=.lo}
.for f in attr.h codes.h config.h emit.c extract.c flags.h opinit.c output.c \
	  peep.c recog.c
CLEANFILES+=		gen${f:R} insn-${f}

gen${f:R}.lo: ${HH}
gen${f:R}: gen${f:R}.lo ${EXTRADEPENDS} 
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY}
insn-${f}: gen${f:R} ${G_md_file}
	${_MKTARGET_CREATE}
	./gen${f:R} ${G_md_file} >${.TARGET}
.endfor

#
# "One of these things is not like the other"
#
CLEANFILES+=		genattrtab insn-attrtab.c
genattrtab.lo: ${HH} gtype-desc.h
genautomata.lo: ${HH}
genattrtab: genattrtab.lo genautomata.lo ${EXTRADEPENDS} ${G_HOST_VARRAY:.o=.lo}
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${NBCOMPATLIB} ${HOSTLIBIBERTY} -lm
insn-attrtab.c: genattrtab ${G_md_file}
	${_MKTARGET_CREATE}
	./genattrtab ${G_md_file} >${.TARGET}

#
# Required hard-coded dependancies.
#
genextract.lo: insn-config.h
insn-conditions.lo: insn-constants.h tm_p.h tm-preds.h

insn-attrtab.o: insn-config.h ${G_HOST_RTLANAL:.o=.c}
insn-extract.o: insn-config.h
toplev.d toplev.o: options.h

.PATH: ${DIST}/gcc ${DIST}/libiberty ${GCCARCH} ${G_out_file:H}
