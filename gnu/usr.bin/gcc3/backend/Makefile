#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:30 mrg Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		backend

SRCS=		${G_OBJS:.o=.c} ${G_out_file:T}

CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DTARGET_NAME=\"${MACHINE_GNU_PLATFORM}\" \
		-DTARGET_CPU_DEFAULT=${G_target_cpu_default}
HOST_CPPFLAGS+=	-I. -I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include <bsd.lib.mk>

#
# Independent generation programs.
#
CLEANFILES+=	gengenrtl gengenrtl.o genrtl.c genrtl.h \
		gengtype gtyp-gen.h gtype-desc.c gtype-desc.h \
		gtype-c.h gtype-cp.h gtype-f.h gtype-objc.h \
		genflags genflags.h \
		gencheck tree-check.h \
		genpreds tm-preds.h tm_p.h cs-tm_p.h \
		auto-host.h print-rtl1.c \
		build-rtl.c build-errors.c build-varray.c build-bitmap.c build-ggc-none.c \
		gt-*.h *.o \
		config.h cs-config.h \

LIBIBERTYOBJ!=	cd ${.CURDIR}/../host-libiberty && ${PRINTOBJDIR}
LIBIBERTY=	${LIBIBERTYOBJ}/libiberty/libiberty.a

#
# These files have special needs
#
COPTS.prefix.o=			-DPREFIX=\"$(prefix)\"
COPTS.toplev.o=			-DTARGET_NAME=\"$(target_alias)\"
COPTS.insn-conditions.c=	-DGENERATOR_FILE

#
# Generate the various header files we need.
#
genrtl.c genrtl.h: gengenrtl
	./gengenrtl > genrtl.c
	./gengenrtl -h > genrtl.h
gengenrtl.o: gengenrtl.c
	${HOST_COMPILE.c} $>
gengenrtl: gengenrtl.o
	${HOST_LINK.c} -o gengenrtl $> ${NBCOMPATLIB} ${LIBIBERTY}

tm-preds.h: genpreds
	./genpreds > tm-preds.h
genpreds.o: genpreds.c
	${HOST_COMPILE.c} $>
genpreds: genpreds.o
	${HOST_LINK.c} -o genpreds $> ${NBCOMPATLIB} ${LIBIBERTY}

#gencheck.o: gencheck.c
#	${HOST_COMPILE.c} $>
tree-check.h: gencheck.c
	${HOST_LINK.c} -o gencheck $> ${NBCOMPATLIB} ${LIBIBERTY}
	./gencheck >$@

gtype-desc.c: gtype-desc.h
gtype-desc.h: gengtype
	rm -f auto-host.h
	ln -s ${.CURDIR}/../arch/${MACHINE_ARCH}/auto-host.h .
	./gengtype
gengtype.c: gtyp-gen.h
gengtype.o: gengtype.c
	${HOST_COMPILE.c} $>
gengtype-lex.o: gengtype-lex.c
	${HOST_COMPILE.c} $>
gengtype-yacc.o: gengtype-yacc.c
	${HOST_COMPILE.c} $>
gengtype: gengtype.o gtyp-gen.h gengtype-lex.o gengtype-yacc.o config.h
	${HOST_LINK.c} -o gengtype gengtype.o gengtype-lex.o gengtype-yacc.o ${NBCOMPATLIB} ${LIBIBERTY}

gtyp-gen.h: Makefile
	(	echo "static const char *srcdir = \"$(G_GTFILES_SRCDIR)\";"		; \
		echo "static const char *lang_files[] = {"				; \
		for f in $(G_GTFILES_FILES_FILES); do echo "\"$$f\", "; done		; \
		echo "NULL };"								; \
		echo "static const char *langs_for_lang_files[] = {"			; \
		for f in $(G_GTFILES_FILES_LANGS); do echo "\"$$f\", "; done 		; \
		echo "NULL };"								; \
		echo "static const char *all_files[] = {"				; \
		for f in $(G_GTFILES); do echo "\"$$f\", "; done			; \
		echo " NULL };"								; \
		echo "static const char *lang_dir_names[] = {"				; \
		echo "\"c\", "								; \
		for f in $(G_GTFILES_LANG_DIR_NAMES); do echo "\"$$f\", "; done		; \
		echo "NULL };"								; \
	) > gtyp-gen.h

# XXX /bin/sh
config.h:
	TM_DEFINES="$(G_tm_defines)" \
	HEADERS="$(G_host_xm_file)" XM_DEFINES="$(G_host_xm_defines)" \
	TARGET_CPU_DEFAULT="$(G_target_cpu_default)" \
	/bin/sh $(GNUHOSTDIST)/gcc/mkconfig.sh config.h

tm_p.h:
	TM_DEFINES="" \
	HEADERS="$(G_tm_p_file)" XM_DEFINES="" TARGET_CPU_DEFAULT="" \
	/bin/sh $(GNUHOSTDIST)/gcc/mkconfig.sh tm_p.h

options.h: Makefile
	for f in $(lang_options_files); do \
		echo "#include \"$$f\""; \
	done | sed 's|$(GNUHOSTDIST)/gcc/||' > options.h

#
# These are copies of files we build for the build host that are used
# by the genfoo programs.
#
build-rtl.c: rtl.c \
  $(G_RTL_H) real.h $(G_GCC_H) errors.h gtype-desc.h
	rm -f build-rtl.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/rtl.c > build-rtl.c
build-rtl.o: build-rtl.c
	${HOST_COMPILE.c} -DGENERATOR_FILE build-rtl.c

print-rtl1.c: print-rtl.c \
  $(G_RTL_H) $(G_TREE_H) hard-reg-set.h $(G_BASIC_BLOCK_H)
	rm -f print-rtl1.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/print-rtl.c > print-rtl1.c
print-rtl1.o: print-rtl.c
	${HOST_COMPILE.c} -DGENERATOR_FILE print-rtl1.c

build-bitmap.c: bitmap.c \
  $(G_RTL_H) flags.h $(G_BASIC_BLOCK_H) $(REGS_H) $(G_GCC_H) gtype-desc.h
	rm -f build-bitmap.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/bitmap.c > build-bitmap.c
build-bitmap.o: build-bitmap.c
	${HOST_COMPILE.c} ${COPTS.${.IMPSRC:T}} build-bitmap.c

build-errors.c: errors.c errors.h
	rm -f build-errors.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/errors.c > build-errors.c
build-errors.o: build-errors.c
	${HOST_COMPILE.c} -DGENERATOR_FILE build-errors.c

build-varray.c: varray.c \
  $(G_RTL_H) $(G_GCC_H) $(TREE_H) bitmap.h errors.h
	rm -f build-varray.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/varray.c > build-varray.c
build-varray.o: build-varray.c
	${HOST_COMPILE.c} ${COPTS.${.IMPSRC:T}} build-varray.c

build-ggc-none.c: ggc-none.c \
  $(G_GCC_H) gtype-desc.h
	rm -f build-ggc-none.c
	sed -e 's/config[.]h/hconfig.h/' ${GNUHOSTDIST}/gcc/ggc-none.c > build-ggc-none.c
build-ggc-none.o: build-ggc-none.c
	${HOST_COMPILE.c} -DGENERATOR_FILE build-ggc-none.c

#
# The `early' insn-foo generators
#
EXTRADEPENDSEARLY=	${G_HOST_EARLY_SUPPORT} ${G_HOST_RTL} ${G_HOST_ERRORS}
.for f in conditions.c constants.h
CLEANFILES+=		gen${f:R} insn-${f}

gen${f:R}.o: gen${f:R}.c
	${HOST_COMPILE.c} $>
insn-${f:R}.o:
	${HOST_COMPILE.c} ${COPTS.${.IMPSRC:T}} insn-${f:R}.c
insn-${f}: genrtl.h ${G_md_file} ${EXTRADEPENDSEARLY:.c=.o} gen${f:R}.o
	${HOST_LINK.c} -o gen${f:R} gen${f:R}.o ${EXTRADEPENDSEARLY:.c=.o} ${NBCOMPATLIB} ${LIBIBERTY}
	./gen${f:R} ${G_md_file} >$@
.endfor

#
# The normal insn-foo generators
#
.for f in gensupport.c read-rtl.c
${f:R}.o: ${f:R}.c
	${HOST_COMPILE.c} $>
.endfor

EXTRADEPENDS=		${G_HOST_RTL} ${G_HOST_SUPPORT} ${G_HOST_PRINT} ${G_HOST_ERRORS}
.for f in attr.h codes.h config.h emit.c extract.c flags.h opinit.c output.c \
	  peep.c recog.c
CLEANFILES+=		gen${f:R} insn-${f}

gen${f:R}.o: gen${f:R}.c
	${HOST_COMPILE.c} $>
insn-${f}: genrtl.h ${G_md_file} ${EXTRADEPENDS:.c=.o} gen${f:R}.o
	${HOST_LINK.c} -o gen${f:R} gen${f:R}.o ${EXTRADEPENDS:.c=.o} ${NBCOMPATLIB} ${LIBIBERTY}
	./gen${f:R} ${G_md_file} >$@
.endfor

#
# "One of these things is not like the other"
#
CLEANFILES+=		genattrtab insn-attrtab.c
genattrtab.o: genattrtab.c
	${HOST_COMPILE.c} $>
genautomata.o: genautomata.c
	${HOST_COMPILE.c} $>
insn-attrtab.c: genrtl.h ${G_md_file} ${EXTRADEPENDS:.c=.o} ${G_HOST_VARRAY:.c=.o} genattrtab.o genautomata.o
	${HOST_LINK.c} -o genattrtab genattrtab.o genautomata.o ${EXTRADEPENDS:.c=.o} ${G_HOST_VARRAY:.c=.o} ${NBCOMPATLIB} ${LIBIBERTY} -lm
	./genattrtab ${G_md_file} >$@

#
# Required hard-coded dependancies.
#
insn-attrtab.c: ${G_HOST_RTLANAL:.o=.c}
insn-attrtab.c insn-extract.c: insn-config.h
insn-conditions.c: insn-constants.h tm_p.h tm-preds.h
toplev.c: options.h
${OBJS} .depend: insn-attr.h insn-codes.h insn-config.h insn-flags.h tree-check.h

.PATH: ${DIST}/gcc ${DIST}/libiberty ${G_out_file:H}
